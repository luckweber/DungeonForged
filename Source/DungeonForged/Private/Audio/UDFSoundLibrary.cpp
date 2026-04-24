// Source/DungeonForged/Private/Audio/UDFSoundLibrary.cpp
#include "Audio/UDFSoundLibrary.h"
#include "Sound/SoundBase.h"

USoundBase* UDFSoundLibrary::GetSoundForTag(const FGameplayTag& Tag) const
{
	if (!Tag.IsValid())
	{
		return nullptr;
	}
	if (const TObjectPtr<USoundBase>* P = TaggedSounds.Find(Tag))
	{
		return P->Get();
	}
	return nullptr;
}

USoundBase* UDFSoundLibrary::GetSound(const UDFSoundLibrary* const Library, const FGameplayTag& Tag)
{
	return Library ? Library->GetSoundForTag(Tag) : nullptr;
}
