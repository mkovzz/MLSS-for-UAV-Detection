# Copilot Instructions

## Project Guidelines
- When reasoning about Unix timestamps, verify the conversion against the user's stated date/current date instead of assuming the timestamp is in the future.
- Treat OpenSky HTTP 429 as a quota/rate-limit error and surface it immediately, rather than assuming it indicates concurrent requests or handling it with automatic repeated retries.