# adhan shared test data (spec 007 research R3)

`Shared/Times/*.json` of adhan-js, tag `v4.4.6`, commit `b6e04cd9da28fffc66773404a0a2e9c739370baf`
(MIT, Batoul Apps; notice in `lib/core/src/nvp/calc/LICENSE-adhan.txt`), copied unchanged on 2026-11-26.
Used by `test/test_calc/test_prayer_times.cpp` as the oracle for the C++ port; a time passes when it is
within the file's `variance` minutes (0 when absent). London (Moonsighting Committee) is not used: that
method's special cases are not ported.
