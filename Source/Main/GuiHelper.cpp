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

#include "GuiHelper.h"
#include "Presets.h"

std::unique_ptr<FileChooser> GuiHelper::m_fileChooser;

class AsyncDialogWindow : public DialogWindow
{
public:
	AsyncDialogWindow(Component* content, const String& title)
		: DialogWindow(title, Colours::lightgrey, true, true)
	{
		bool usingNativeTitleBar = (SystemStats::getOperatingSystemType() & SystemStats::Windows) ||
			(SystemStats::getOperatingSystemType() & SystemStats::MacOSX);
		setUsingNativeTitleBar(usingNativeTitleBar);
		setContentOwned(content, true);
		centreAroundComponent(nullptr, getWidth(), getHeight());
		setResizable(false, false);
	}

	~AsyncDialogWindow()
	{
		FadeOut();
	}

	void closeButtonPressed() override
	{
		setVisible (false);
	}

	void Launch()
	{
		FadeIn();
		enterModalState(true, nullptr, true);
	}

	void FadeIn()
	{
		m_topWindow = TopLevelWindow::getActiveTopLevelWindow();
		m_topWindow->getChildren()[0]->setAlpha(0.3);
	}

	void FadeOut()
	{
		if (m_topWindow)
		{
			m_topWindow->getChildren()[0]->setAlpha(1.0);
		}
	}

private:
	TopLevelWindow* m_topWindow = nullptr;
};

void GuiHelper::Final()
{
	m_fileChooser.reset();
}

void GuiHelper::CallAsync(Component* component, std::function<void()> function)
{
	Component::SafePointer<Component> safeComponent(component);
	MessageManager::callAsync([safeComponent, function]()
		{
			if (safeComponent != nullptr)
			{
				function();
			}
		});
}

void GuiHelper::ShowDialogAsync(Component* content, const String& title)
{
	AsyncDialogWindow* dialog = new AsyncDialogWindow(content, title);
	dialog->Launch();
}

void GuiHelper::ShowFileOpenDialogAsync(const String& title, const String& initialLocation,
	const String& patterns, std::function<void(const URL&)> callback)
{
	File location(initialLocation);
	m_fileChooser = std::make_unique<FileChooser>(title, location, patterns);

	m_fileChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
		[callback](const FileChooser& ch)
		{
			const URL url = ch.getURLResult();
			if (!url.isEmpty())
			{
				callback(url);
			}
		});
}

void GuiHelper::ShowFileSaveDialogAsync(const String& title, const String& initialLocation,
	const String& patterns, std::function<void(const URL&)> callback)
{
	File location(initialLocation);
	m_fileChooser = std::make_unique<FileChooser>(title, location, patterns);

	// warnAboutOverwriting: the dialog asks for confirmation before replacing an existing file
	m_fileChooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles |
		FileBrowserComponent::warnAboutOverwriting,
		[callback](const FileChooser& ch)
		{
			const URL url = ch.getURLResult();
			if (!url.isEmpty())
			{
				callback(url);
			}
		});
}

void GuiHelper::ShowMenuAsync(PopupMenu& menu, Component* comp,
	std::function<void(int)> callback)
{
	menu.showMenuAsync(PopupMenu::Options()
		.withTargetComponent(comp)
		.withStandardItemHeight(35),
		callback);
}

PopupMenu GuiHelper::BuildVoicesMenu(Voice* currentVoice, int idBase)
{
	PopupMenu voicesMenu;
	PopupMenu category1Menu;
	PopupMenu category2Menu;
	String category1;
	String category2;

	auto flushCategory2 = [&]()
	{
		if (category2Menu.getNumItems() > 0)
		{
			category1Menu.addSubMenu(category2, category2Menu);
		}
		category2Menu.clear();
	};

	auto flushCategory1 = [&]()
	{
		flushCategory2();
		if (category1Menu.getNumItems() > 0)
		{
			voicesMenu.addSubMenu(category1, category1Menu);
		}
		category1Menu.clear();
	};

	VoiceList& voices = Presets::Voices();
	for (int i = 0; i < (int)voices.size(); i++)
	{
		Voice& vc = voices[i];
		if (vc.category1 != category1)
		{
			flushCategory1();
			category1 = vc.category1;
			category2 = vc.category2;
		}
		else if (vc.category2 != category2)
		{
			flushCategory2();
			category2 = vc.category2;
		}
		category2Menu.addItem(idBase + i, vc.title, true, &vc == currentVoice);
	}
	flushCategory1();

	return voicesMenu;
}

void GuiHelper::DrawOtsIcon(Graphics& g, Rectangle<float> area, float lineWidth)
{
	// drawn on a 22 x 22 grid, like the icons of the top bar
	const float scale = jmin(area.getWidth(), area.getHeight()) / 22.0f;
	const float x0 = area.getCentreX() - 11.0f * scale;
	const float y0 = area.getCentreY() - 11.0f * scale;
	const float size = 8.5f * scale;
	const float corner = 1.5f * scale;
	const float positions[4][2] = { { 1.5f, 1.5f }, { 12.0f, 1.5f }, { 1.5f, 12.0f }, { 12.0f, 12.0f } };
	for (int i = 0; i < 4; i++)
	{
		const Rectangle<float> r(x0 + positions[i][0] * scale, y0 + positions[i][1] * scale, size, size);
		if (i == 0)
		{
			g.fillRoundedRectangle(r, corner);
		}
		else
		{
			g.drawRoundedRectangle(r.reduced(lineWidth / 2), corner, lineWidth);
		}
	}
}
