#include "utils.h"

int GetCoverScore(const QString & str_)
{
	QString str = str_.toLower();
	int score = 0;
	if (str.contains("0001"))
		score += 1001;
	else if (str.contains("0000"))
		score += 1000;
	else if (str.contains("001"))
		score += 101;
	else if (str.contains("000"))
		score += 100;
	else if (str.contains("01"))
		score += 11;
	else if (str.contains("00"))
		score += 10;
	else if (str.contains("1"))
		score += 1;

	if (str.contains("art"))
		score += 10000;
	if (str.contains("album"))
		score += 10000;
	if (str.contains("cover"))
		score += 20000;
	if (str.contains("large"))
		score += 20000;
	if (str.contains("small"))
		score += 10000;
	if (str.contains("folder"))
		score += 10000;
	return score;
}
