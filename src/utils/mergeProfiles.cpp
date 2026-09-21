#include "./mergeProfiles.hpp"

Profile mergeProfiles(
    const Profile &oldProfile,
    const Profile &newProfile,
    bool /*automaticallyCloseRuns*/)
{
    Profile result = oldProfile;

    result.id = oldProfile.id;
    result.profileName = oldProfile.profileName;
    result.data.tags = newProfile.data.tags;
    // Repeated cycles must never borrow another cycle's progress.
    result.data.stages = bacon::mergeCycleRanges(oldProfile.data.stages, newProfile.data.stages);

    result.data.fixedPartsVersion = 1;
    bacon::refreshCompletion(result);
    return result;
}