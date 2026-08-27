-- Reference to documents/schemas/supabase_telemetry_schema.sql
-- Run this SQL in your Supabase SQL Editor to initialize the database table and hourly automated aggregation.

-- 1. RAW TELEMETRY TABLE
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

CREATE INDEX IF NOT EXISTS idx_telemetry_created_at_desc ON public.telemetry (created_at DESC);

-- 2. HOURLY AGGREGATED ANALYTICS TABLE
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

-- 3. RLS POLICIES
ALTER TABLE public.telemetry ENABLE ROW LEVEL SECURITY;
ALTER TABLE public.telemetry_hourly ENABLE ROW LEVEL SECURITY;

CREATE POLICY "Allow public read telemetry" ON public.telemetry FOR SELECT TO anon, authenticated USING (true);
CREATE POLICY "Allow public read hourly analytics" ON public.telemetry_hourly FOR SELECT TO anon, authenticated USING (true);
CREATE POLICY "Allow device insert telemetry" ON public.telemetry FOR INSERT TO anon, authenticated WITH CHECK (true);
