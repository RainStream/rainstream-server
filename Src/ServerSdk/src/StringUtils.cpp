#include "RainStream.hpp"
#include "StringUtils.hpp"

std::string join(const AStringVector& vec, const std::string & delimeter)
{
	if (vec.empty())
	{
		return {};
	}

	// Do a dry run to gather the size
	const auto DelimSize = delimeter.size();
	size_t ResultSize = vec.at(0).size();
	std::for_each(vec.begin() + 1, vec.end(),
		[&](const std::string & a_String)
	{
		ResultSize += DelimSize;
		ResultSize += a_String.size();
	}
	);

	// Now do the actual join
	std::string Result;
	Result.reserve(ResultSize);
	Result.append(vec.at(0));
	std::for_each(vec.begin() + 1, vec.end(),
		[&](const std::string & a_String)
	{
		Result += delimeter;
		Result += a_String;
	}
	);
	return Result;
}


