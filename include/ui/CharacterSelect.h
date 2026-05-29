#ifndef CHARACTER_SELECT_H_
#define CHARACTER_SELECT_H_
// Phase 4 — Persona / kPersonas / CharacterSelectResult moved into
// game/entities/Personas.h (game-domain model data, not UI). This
// header now just re-exports them via a transitive include for
// source-compatibility with the scenes / tests that consume them via
// the ui/ path. New code should include game/entities/Personas.h
// directly.
#include "game/entities/Personas.h"

#endif // CHARACTER_SELECT_H_
