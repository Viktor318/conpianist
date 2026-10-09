/*
 *  This file is part of ConPianist. See <https://github.com/hugbug/conpianist>.
 *
 *  Copyright (C) 2018 Andrey Prygunkov <hugbug@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PianoController.h"
#include "Settings.h"

class ScoreComponent : public Component
{
public:
	static ScoreComponent* Create(Settings& settings, PianoController& pianoController);

	// A score is drawn by a separate process before it is shown (so a faulty score cannot
	// freeze or crash the program): the program is started with this argument, the path of
	// the score and the path of a result file. CheckScoreFile does the drawing in that
	// process; its result is the exit code of the process: 0 if the score could be drawn,
	// CheckScoreError (with the reason written into the result file) if Lomse reported an error.
	static constexpr const char* CheckScoreArgument = "--check-score";
	static constexpr int CheckScoreError = 10;
	static int CheckScoreFile(const File& file, const File& resultFile);
};
