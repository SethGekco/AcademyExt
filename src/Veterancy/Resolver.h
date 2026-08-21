#pragma once

#include <algorithm>

// Accumulates veterancy contributions and resolves them under the AcademyExt
// rule:
//
//     result = max( best non-stacking contribution, sum of stacking ones )
//
// So three academies A (non-stacking), B and C (stacking) yield max(A, B+C):
// whichever is genuinely better, the standalone bonus or the combined stack.
//
// TWO INVARIANTS -- see DESIGN.md section 4. Breaking either one voids the proof
// that AcademyExt's result is always >= Antares' plain max(), and that proof is
// the only reason it is safe for both DLLs to hook the same addresses:
//
//   1. The result is never negative. Antares implicitly clamps at zero by
//      seeding its accumulator at 0.0; a negative StackSum here must never win.
//   2. Callers must only ever RAISE an existing veterancy value, never lower it.
//      Consequence, accepted deliberately: a "cap" or "override" academy that
//      reduces veterancy is impossible under this architecture.
class VeterancyResolver
{
public:
	void Add(double value, bool stacks)
	{
		if (stacks)
			this->StackSum += value;
		else
			this->BestSingle = std::max(this->BestSingle, value);
	}

	// cap is RulesClass::Instance->VeteranCap.
	double Resolve(double cap) const
	{
		double const combined = std::max(this->BestSingle, this->StackSum);

		// The lower clamp is explicit even though BestSingle seeds at 0.0.
		// A negative Academy.*Veterancy is expressible in INI, and without this
		// a negative StackSum could surface as the result -- invariant 1.
		return std::clamp(combined, 0.0, cap);
	}

	bool Empty() const
	{
		return this->BestSingle == 0.0 && this->StackSum == 0.0;
	}

private:
	double BestSingle = 0.0; // max over contributions with stacks == false
	double StackSum = 0.0;   // sum over contributions with stacks == true
};
