#pragma once

#include "../serialization/profile/index.hpp"

// Returns a copy of the profile with fields stripped according to the
// provided export categories. Identifiers (ids, stage numbers, range
// bounds/consider) are always kept since they're required for the data
// to be re-importable.
Profile filterProfileForExport(
    Profile const &profile,
    bool includeSecrets,
    bool includeProgression,
    bool includeAttempts,
    bool includeNotes);
