# Shermanish v0.2 — NTS-3 resonator + tempo envelopes

Sherman Filterbank-inspired **dual resonant bandpass** effect, not a circuit-accurate model. Designed for one baritone sax microphone and the Zoom L6max's separately mixed sax and loop sends.

## Routing

Sax mic → L6max dry channel. AUX 1 → looper + reverse/echo → L6max return. Mix the dry sax and loop return at independently adjustable SUB levels → stereo SUB OUT → NTS-3 → L6max return → master speakers. **Do not route the NTS-3 return to SUB OUT.**

NTS-3 receives a stereo signal (possibly dual mono); modest opposed detuning inside the algorithm creates stereo movement from the mono mic.

## Controls

| Control | Parameter | What it does |
|---|---|---|
| X | FREQ (0–1000) | Logarithmic main filter sweep, about 55–2500 Hz |
| Y | INTERVAL (−240…+240) | Filter 2 detuning relative to filter 1, semitones/10 |
| FX DEPTH | REGEN (0–1000) | Resonance plus local saturated feedback |
| Extra 1 | DRIVE (0–1000) | Input distortion |
| Extra 2 | ROUTING (0–2) | 0 parallel, 1 serial, 2 cross-coupled |
| Extra 3 | ENV AMT (0–1000) | Envelope-to-filter frequency range, plus pumping strength in tempo modes |
| Extra 4 | ENV MODE (0–9) | Select envelope algorithm and tempo division |
| Extra 5 | DRY WET (−1000…1000) | True dry/wet, defaults fully wet |

### Envelope modes

| Number | Name | Effect |
|---|---|---|
| 0 | OFF | No envelope modulation |
| 1 | FOLLOW FAST | Quick audio envelope follower (default) |
| 2 | FOLLOW SLOW | Smoother slower swell follower |
| 3 | TRIG PUNCH | Audio-triggered fast attack + ADSR decay/release |
| 4 | TRIG SWELL | Audio-triggered slow attack + long release |
| 5 | CLOCK 1/16 | Tempo-synced filter and wet amplitude envelope every sixteenth note |
| 6 | CLOCK 1/8 | Every eighth note |
| 7 | CLOCK 1/4 | Every quarter note |
| 8 | CLOCK 1/2 | Every half note |
| 9 | CLOCK 1 BAR | Every four beats (4/4 bar) |

The NTS-3 tempo button can set BPM by tapping three or more times. The SDK supplies tempo-change and 4-PPQN clock callbacks. Clock modes use those callbacks to align the pulse and also run from current BPM between ticks. The amplitude envelope in the tempo modes never fully silences the wet path; it can be dialed down using ENV AMT. Envelope followers and triggered ADSR respond to the actual incoming L6max SUB mix (live sax + loops).

This initial implementation shares envelope modulation between both filters. The original Sherman has separate modulation amounts and an independent AR output envelope; we can consider a more faithful dual-filter/modulation implementation after listening tests. Slow wobble from v0.1 has been replaced by ENV MODE to keep all controls in one NTS-3 effect (8 params).

### First test

Begin with only FX1 enabled. Start with FREQ ≈ 360, INTERVAL = 0, REGEN ≈ 350–450, DRIVE ≈ 74, ROUTING = 0, ENV AMT ≈ 450, ENV MODE = 1, fully wet. Feed a sustained bari loop while sweeping X. Switch ENV MODE to 3 or 4 to test attack/release following. Then set tempo with three or more taps and choose mode 7 (1/4) to hear rhythmic pumping. Raise REGEN gradually. **Begin at low monitor volume**; high resonance creates loud sustained pitches.

## Build and tests

`./docker/run_cmd.sh build nts-3_kaoss/shermanish`

Output: `platform/nts-3_kaoss/shermanish/shermanish.nts3unit`

`g++ -std=c++11 -O2 -Iplatform/nts-3_kaoss/common platform/nts-3_kaoss/shermanish/test_host.cc -o /tmp/shermanish-test && /tmp/shermanish-test`

The GitHub Actions workflow runs the host tests and builds the .nts3unit file. This is still an experimental prototype; hardware tone, CPU load and edge cases require listening tests.
