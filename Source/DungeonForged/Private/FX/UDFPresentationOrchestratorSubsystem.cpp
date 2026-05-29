// Source/DungeonForged/Private/FX/UDFPresentationOrchestratorSubsystem.cpp
#include "FX/UDFPresentationOrchestratorSubsystem.h"
#include "FX/UDFCombatFeedbackTypes.h"

int32 UDFPresentationOrchestratorSubsystem::DispatchFromHitContext(const FDFHitConfirmedContext& Context)
{
	FDFPresentationMoment Moment;
	Moment.MomentId = ++LastMomentId;
	Moment.HitBand = Context.Band;
	Moment.DamagePercent = Context.DamagePercent;
	Moment.bWasCritical = Context.bIsCrit;
	Moment.bWasLethal = Context.bWasLethal;
	Moment.Instigator = Context.Instigator;
	Moment.Victim = Context.Victim;
	Moment.Location = Context.Location;
	OnPresentationMoment.Broadcast(Moment);
	return Moment.MomentId;
}
