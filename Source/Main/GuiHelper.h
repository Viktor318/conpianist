/*
 *  This file is part of ConPianist. See <https://github.com/Viktor318/conpianist>.
 *  Fork of the original project <https://github.com/hugbug/conpianist>.
 *
 *  Copyright (C) 2020 Andrey Prygunkov <hugbug@users.sourceforge.net>
 *  Copyright (C) 2026 Viktor Oszkó <oszko.viktor@gmail.com>
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

class GuiHelper
{
public:
	static void ShowDialogAsync(Component* content, const String& title);

	static void ShowFileOpenDialogAsync(const String& title,
		const String& initialLocation,
        const String& patterns,
        std::function<void(const URL&)> callback);

	static void ShowFileSaveDialogAsync(const String& title,
		const String& initialLocation,
        const String& patterns,
        std::function<void(const URL&)> callback);

	static void ShowMenuAsync(PopupMenu& menu, Component* comp,
		std::function<void(int)> callback);

	// Runs the function later on the message thread, but only if the component
	// still exists then (it may be deleted before the call is delivered).
	static void CallAsync(Component* component, std::function<void()> function);

	static void Final();
	
private:
	static std::unique_ptr<FileChooser> m_fileChooser;
};
