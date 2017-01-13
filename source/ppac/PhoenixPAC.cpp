#include "PhoenixPAC.h"

using namespace PPAC;

TPUID TPUID::operator+(const TPUID& rhs) const
{
	auto ret = *this;
	ret.t += rhs.t;
	ret.p += rhs.p;
	ret.u += rhs.u;
	return ret;
}

TPUID TPUID::operator-(const TPUID& rhs) const
{
	auto ret = *this;
	ret.t -= rhs.t;
	ret.p -= rhs.p;
	ret.u -= rhs.u;
	return ret;
}

TPUID TPUID::operator^(const TPUID& rhs) const
{
	auto ret = *this;
	ret.tpuid ^= rhs.tpuid;
	return ret;
}

TPUID TPUID::operator|(const TPUID& rhs) const
{
	auto ret = *this;
	ret.tpuid |= rhs.tpuid;
	return ret;
}

TPUID TPUID::operator&(const TPUID& rhs) const
{
	auto ret = *this;
	ret.tpuid &= rhs.tpuid;
	return ret;
}

TPUID TPUID::operator~() const
{
	auto ret = *this;
	ret.tpuid = ~ret.tpuid;
	return ret;
}

TPUID& TPUID::operator+=(const TPUID& rhs)
{
	this->t += rhs.t;
	this->p += rhs.p;
	this->u += rhs.u;
	return *this;
}

TPUID& TPUID::operator-=(const TPUID& rhs)
{
	this->t -= rhs.t;
	this->p -= rhs.p;
	this->u -= rhs.u;
	return *this;
}

TPUID& TPUID::operator&=(const TPUID& rhs)
{
	this->tpuid &= rhs.tpuid;
	return *this;
}

TPUID& TPUID::operator|=(const TPUID& rhs)
{
	this->tpuid |= rhs.tpuid;
	return *this;
}

TPUID& TPUID::operator^=(const TPUID& rhs)
{
	this->tpuid ^= rhs.tpuid;
	return *this;
}

bool PPAC::operator==(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid == rhs.tpuid;
}

bool PPAC::operator!=(const TPUID& lhs, const TPUID& rhs)
{
	return !(lhs == rhs);
}

bool PPAC::operator<(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid < rhs.tpuid;
}

bool PPAC::operator>(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid > rhs.tpuid;
}

bool PPAC::operator>=(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid >= rhs.tpuid;
}

bool PPAC::operator<=(const TPUID& lhs, const TPUID& rhs)
{
	return lhs.tpuid <= rhs.tpuid;
}
