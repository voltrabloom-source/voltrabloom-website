-- ==============================================================================
-- VOLTRABLOOM SUPABASE DATABASE SCHEMA & AUTOMATED AGGREGATION
-- Hybrid Energy Harvesting & Management System (HEMS)
-- ==============================================================================

-- 1. RAW TELEMETRY TABLE (Streams raw 5-second sensor updates)
CREATE TABLE IF NOT EXISTS public.telemetry (
    id              BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    solar_v         FLOAT NOT NULL DEFAULT 0.0,
    wind_v          FLOAT NOT NULL DEFAULT 0.0,
    soil_v          FLOAT NOT NULL DEFAULT 0.0,
    output_v        FLOAT NOT NULL DEFAULT 0.0,
    current_in_a    FLOAT NOT NULL DEFAULT 0.0,
    current_out_a   FLOAT NOT NULL DEFAULT 0.0,
    battery_percent FLOAT NOT NULL DEFAULT 0.0,
    battery_ah      FLOAT NOT NULL DEFAULT 0.0
);

-- Optimization Index: Fast retrieval of latest telemetry for web dashboards
CREATE INDEX IF NOT EXISTS idx_telemetry_created_at_desc ON public.telemetry (created_at DESC);

-- 2. HOURLY AGGREGATED ANALYTICS TABLE (Long-Term Storage)
CREATE TABLE IF NOT EXISTS public.telemetry_hourly (
    bucket_start        TIMESTAMPTZ PRIMARY KEY,
    avg_solar_v         FLOAT,
    max_solar_v         FLOAT,
    avg_wind_v          FLOAT,
    max_wind_v          FLOAT,
    avg_soil_v          FLOAT,
    avg_battery_soc     FLOAT,
    min_battery_soc     FLOAT,
    total_energy_wh     FLOAT,
    sample_count        INT NOT NULL DEFAULT 0,
    created_at          TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_telemetry_hourly_bucket ON public.telemetry_hourly (bucket_start DESC);

-- 3. AUTOMATED AGGREGATION FUNCTION
-- Summarizes raw 5-second rows into hourly buckets for long-term historical analytics
CREATE OR REPLACE FUNCTION public.aggregate_telemetry_hourly()
RETURNS void
LANGUAGE plpgsql
SECURITY DEFINER
AS $$
BEGIN
    INSERT INTO public.telemetry_hourly (
        bucket_start,
        avg_solar_v,
        max_solar_v,
        avg_wind_v,
        max_wind_v,
        avg_soil_v,
        avg_battery_soc,
        min_battery_soc,
        total_energy_wh,
        sample_count
    )
    SELECT
        date_trunc('hour', created_at) AS bucket_start,
        ROUND(AVG(solar_v)::numeric, 2) AS avg_solar_v,
        ROUND(MAX(solar_v)::numeric, 2) AS max_solar_v,
        ROUND(AVG(wind_v)::numeric, 2) AS avg_wind_v,
        ROUND(MAX(wind_v)::numeric, 2) AS max_wind_v,
        ROUND(AVG(soil_v)::numeric, 2) AS avg_soil_v,
        ROUND(AVG(battery_percent)::numeric, 1) AS avg_battery_soc,
        ROUND(MIN(battery_percent)::numeric, 1) AS min_battery_soc,
        -- Calculated energy: sum(Voltage * Current * dt_hours)
        ROUND(SUM((solar_v * current_in_a + wind_v * 0.2) * (5.0 / 3600.0))::numeric, 2) AS total_energy_wh,
        COUNT(*) AS sample_count
    FROM public.telemetry
    WHERE created_at < date_trunc('hour', NOW())
      AND created_at >= (NOW() - INTERVAL '48 hours')
    GROUP BY date_trunc('hour', created_at)
    ON CONFLICT (bucket_start) DO UPDATE SET
        avg_solar_v = EXCLUDED.avg_solar_v,
        max_solar_v = EXCLUDED.max_solar_v,
        avg_wind_v = EXCLUDED.avg_wind_v,
        max_wind_v = EXCLUDED.max_wind_v,
        avg_soil_v = EXCLUDED.avg_soil_v,
        avg_battery_soc = EXCLUDED.avg_battery_soc,
        min_battery_soc = EXCLUDED.min_battery_soc,
        total_energy_wh = EXCLUDED.total_energy_wh,
        sample_count = EXCLUDED.sample_count;
END;
$$;

-- 4. AUTOMATED RETENTION POLICY (Prunes raw 5-second rows older than 30 days)
CREATE OR REPLACE FUNCTION public.cleanup_old_raw_telemetry(days_to_keep INT DEFAULT 30)
RETURNS INT
LANGUAGE plpgsql
SECURITY DEFINER
AS $$
DECLARE
    deleted_rows INT;
BEGIN
    DELETE FROM public.telemetry
    WHERE created_at < NOW() - (days_to_keep || ' days')::INTERVAL;
    
    GET DIAGNOSTICS deleted_rows = ROW_COUNT;
    RETURN deleted_rows;
END;
$$;

-- 5. ROW LEVEL SECURITY (RLS) CONFIGURATION
ALTER TABLE public.telemetry ENABLE ROW LEVEL SECURITY;
ALTER TABLE public.telemetry_hourly ENABLE ROW LEVEL SECURITY;

-- Allow anonymous read for dashboards and visualizers
CREATE POLICY "Allow public read telemetry" 
ON public.telemetry FOR SELECT 
TO anon, authenticated 
USING (true);

CREATE POLICY "Allow public read hourly analytics" 
ON public.telemetry_hourly FOR SELECT 
TO anon, authenticated 
USING (true);

-- Allow device upload inserts
CREATE POLICY "Allow device insert telemetry" 
ON public.telemetry FOR INSERT 
TO anon, authenticated 
WITH CHECK (true);
