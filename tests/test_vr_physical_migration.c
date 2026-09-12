#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
static unsigned sConfigVrPhysicalActionsVersion;
static bool configVrPhysicalJumping,configVrPhysicalSwimming,configVrPhysicalPunching;
static bool configVrPhysicalGrabbing,configVrPhysicalClimbing,configVrSwingClimbRelease;
static bool configVrMotionControlledDive,configVrMotionControlledGroundDive;
static bool configVrPhysicalCrouching;
#include "../build/test_vr_physical_migration.inc"
int main(void) {
    assert(configfile_migrate_vr_physical_actions());
    assert(configVrPhysicalJumping && configVrPhysicalSwimming && configVrPhysicalPunching);
    assert(configVrPhysicalGrabbing && configVrPhysicalClimbing && configVrSwingClimbRelease);
    assert(configVrMotionControlledDive && configVrMotionControlledGroundDive);
    assert(!configVrPhysicalCrouching && sConfigVrPhysicalActionsVersion==1);
    configVrPhysicalJumping=false; configVrPhysicalSwimming=false;
    configVrPhysicalPunching=false; configVrPhysicalCrouching=true;
    assert(!configfile_migrate_vr_physical_actions());
    assert(!configVrPhysicalJumping && !configVrPhysicalSwimming && !configVrPhysicalPunching);
    assert(configVrPhysicalCrouching);
    sConfigVrPhysicalActionsVersion=0;
    assert(configfile_migrate_vr_physical_actions()); assert(configVrPhysicalCrouching);
    puts("PASS: one-time physical actions migration, either crouch preference preserved, later disables retained.");
}
