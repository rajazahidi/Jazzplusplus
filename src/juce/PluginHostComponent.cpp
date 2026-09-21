//*****************************************************************************
// The JAZZ++ Midi Sequencer — JUCE 8 Edition
//
// Copyright (C) 2026 Raja Zahidi
// VST3 & CLAP Plugin Host Implementation
//*****************************************************************************

#include "PluginHostComponent.h"

PluginHostComponent::PluginHostComponent(AudioMidiEngine& eng)
    : engine(eng)
{
    // Configure Table
    tablePlugins.setModel(this);
    tablePlugins.getHeader().addColumn("FORMAT",   1, 80, 60, 100);
    tablePlugins.getHeader().addColumn("NAME",     2, 180, 100, 300);
    tablePlugins.getHeader().addColumn("CATEGORY", 3, 140, 80, 200);
    tablePlugins.getHeader().addColumn("VENDOR",   4, 120, 80, 200);
    tablePlugins.getHeader().addColumn("VERSION",  5, 70, 50, 100);
    tablePlugins.getHeader().addColumn("PATH",     6, 320, 150, 600);
    tablePlugins.setColour(juce::ListBox::backgroundColourId, JazzLookAndFeel::Colors::panelBackground);
    addAndMakeVisible(tablePlugins);

    // Filter & search
    addAndMakeVisible(btnFilterAll);
    addAndMakeVisible(btnFilterVst3);
    addAndMakeVisible(btnFilterClap);
    btnFilterAll.setToggleState(true, juce::dontSendNotification);

    btnFilterAll.onClick = [this] {
        activeFilter = eFilterAll;
        btnFilterAll.setToggleState(true, juce::dontSendNotification);
        btnFilterVst3.setToggleState(false, juce::dontSendNotification);
        btnFilterClap.setToggleState(false, juce::dontSendNotification);
        updateFilteredList();
    };

    btnFilterVst3.onClick = [this] {
        activeFilter = eFilterVst3;
        btnFilterAll.setToggleState(false, juce::dontSendNotification);
        btnFilterVst3.setToggleState(true, juce::dontSendNotification);
        btnFilterClap.setToggleState(false, juce::dontSendNotification);
        updateFilteredList();
    };

    btnFilterClap.onClick = [this] {
        activeFilter = eFilterClap;
        btnFilterAll.setToggleState(false, juce::dontSendNotification);
        btnFilterVst3.setToggleState(false, juce::dontSendNotification);
        btnFilterClap.setToggleState(true, juce::dontSendNotification);
        updateFilteredList();
    };

    txtSearch.setTextToShowWhenEmpty("Search plugins by name, category, or vendor...", JazzLookAndFeel::Colors::textSecondary);
    txtSearch.onTextChange = [this] { updateFilteredList(); };
    addAndMakeVisible(txtSearch);

    // Action buttons
    btnScan.onClick = [this] { scanPlugins(); };
    btnLoadFile.onClick = [this] { loadPluginFromFile(); };
    addAndMakeVisible(btnScan);
    addAndMakeVisible(btnLoadFile);

    btnLoadAsInst.onClick = [this] { loadSelectedPlugin(true); };
    btnLoadAsFx.onClick = [this] { loadSelectedPlugin(false); };
    addAndMakeVisible(btnLoadAsInst);
    addAndMakeVisible(btnLoadAsFx);

    // Active slots
    addAndMakeVisible(grpInstrument);
    lblInstName.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    lblInstName.setColour(juce::Label::textColourId, JazzLookAndFeel::Colors::neonCyan);
    lblInstName.setText("No Instrument Loaded", juce::dontSendNotification);
    addAndMakeVisible(lblInstName);

    btnInstBypass.onClick = [this] { engine.setInstrumentBypassed(btnInstBypass.getToggleState()); };
    btnInstOpenUI.onClick = [this] { openPluginEditor(true); };
    btnInstUnload.onClick = [this] { engine.unloadPlugin(true); updateSlotDisplays(); };
    addAndMakeVisible(btnInstBypass);
    addAndMakeVisible(btnInstOpenUI);
    addAndMakeVisible(btnInstUnload);

    addAndMakeVisible(grpEffect);
    lblFxName.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    lblFxName.setColour(juce::Label::textColourId, JazzLookAndFeel::Colors::neonEmerald);
    lblFxName.setText("No Master Effect Loaded", juce::dontSendNotification);
    addAndMakeVisible(lblFxName);

    btnFxBypass.onClick = [this] { engine.setEffectBypassed(btnFxBypass.getToggleState()); };
    btnFxOpenUI.onClick = [this] { openPluginEditor(false); };
    btnFxUnload.onClick = [this] { engine.unloadPlugin(false); updateSlotDisplays(); };
    addAndMakeVisible(btnFxBypass);
    addAndMakeVisible(btnFxOpenUI);
    addAndMakeVisible(btnFxUnload);

    lblStatus.setFont(11.0f);
    lblStatus.setColour(juce::Label::textColourId, JazzLookAndFeel::Colors::textSecondary);
    addAndMakeVisible(lblStatus);

    updateFilteredList();
    updateSlotDisplays();
}

void PluginHostComponent::updateFilteredList()
{
    filteredPlugins.clear();
    const auto& list = engine.getKnownPluginList().getTypes();
    juce::String query = txtSearch.getText().trim();

    for (const auto& desc : list)
    {
        if (activeFilter == eFilterVst3 && !desc.pluginFormatName.equalsIgnoreCase("VST3"))
            continue;
        if (activeFilter == eFilterClap && !desc.pluginFormatName.equalsIgnoreCase("CLAP"))
            continue;

        if (query.isNotEmpty())
        {
            bool matches = desc.name.containsIgnoreCase(query) ||
                           desc.category.containsIgnoreCase(query) ||
                           desc.manufacturerName.containsIgnoreCase(query);
            if (!matches)
                continue;
        }

        filteredPlugins.push_back(desc);
    }

    lblStatus.setText("Total Scanned: " + juce::String(list.size()) +
                      " | Showing: " + juce::String(filteredPlugins.size()) +
                      " (VST3 & CLAP)", juce::dontSendNotification);

    tablePlugins.updateContent();
    tablePlugins.repaint();
}

void PluginHostComponent::updateSlotDisplays()
{
    auto* inst = engine.getActiveInstrumentPlugin();
    if (inst != nullptr)
    {
        lblInstName.setText(inst->getName() + " [" + inst->getPluginDescription().pluginFormatName + "]",
                            juce::dontSendNotification);
        btnInstOpenUI.setEnabled(true);
        btnInstUnload.setEnabled(true);
        btnInstBypass.setEnabled(true);
    }
    else
    {
        lblInstName.setText("No Instrument Loaded", juce::dontSendNotification);
        btnInstOpenUI.setEnabled(false);
        btnInstUnload.setEnabled(false);
        btnInstBypass.setEnabled(false);
    }

    auto* fx = engine.getActiveEffectPlugin();
    if (fx != nullptr)
    {
        lblFxName.setText(fx->getName() + " [" + fx->getPluginDescription().pluginFormatName + "]",
                          juce::dontSendNotification);
        btnFxOpenUI.setEnabled(true);
        btnFxUnload.setEnabled(true);
        btnFxBypass.setEnabled(true);
    }
    else
    {
        lblFxName.setText("No Master Effect Loaded", juce::dontSendNotification);
        btnFxOpenUI.setEnabled(false);
        btnFxUnload.setEnabled(false);
        btnFxBypass.setEnabled(false);
    }
}

int PluginHostComponent::getNumRows()
{
    return static_cast<int>(filteredPlugins.size());
}

void PluginHostComponent::paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(JazzLookAndFeel::Colors::controlFill.brighter(0.2f));
    else if (rowNumber % 2 == 1)
        g.fillAll(JazzLookAndFeel::Colors::panelBackground.darker(0.2f));
}

void PluginHostComponent::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(filteredPlugins.size()))
        return;

    const auto& desc = filteredPlugins[static_cast<size_t>(rowNumber)];
    g.setFont(11.0f);

    juce::Rectangle<int> cell(4, 0, width - 8, height);

    if (columnId == 1) // FORMAT
    {
        bool isVst3 = desc.pluginFormatName.equalsIgnoreCase("VST3");
        juce::Colour c = isVst3 ? JazzLookAndFeel::Colors::neonCyan : JazzLookAndFeel::Colors::neonEmerald;
        g.setColour(c.withAlpha(0.2f));
        g.fillRoundedRectangle(cell.reduced(2, 4).toFloat(), 3.0f);
        g.setColour(c);
        g.drawRoundedRectangle(cell.reduced(2, 4).toFloat(), 3.0f, 1.0f);
        g.drawText(desc.pluginFormatName, cell, juce::Justification::centred);
    }
    else if (columnId == 2) // NAME
    {
        g.setColour(JazzLookAndFeel::Colors::textPrimary);
        g.drawText(desc.name, cell, juce::Justification::centredLeft, true);
    }
    else if (columnId == 3) // CATEGORY
    {
        g.setColour(JazzLookAndFeel::Colors::textSecondary);
        g.drawText(desc.category, cell, juce::Justification::centredLeft, true);
    }
    else if (columnId == 4) // VENDOR
    {
        g.setColour(JazzLookAndFeel::Colors::textSecondary);
        g.drawText(desc.manufacturerName, cell, juce::Justification::centredLeft, true);
    }
    else if (columnId == 5) // VERSION
    {
        g.setColour(JazzLookAndFeel::Colors::textSecondary);
        g.drawText(desc.version, cell, juce::Justification::centredLeft, true);
    }
    else if (columnId == 6) // PATH
    {
        g.setColour(JazzLookAndFeel::Colors::textDimmed);
        g.drawText(desc.fileOrIdentifier, cell, juce::Justification::centredLeft, true);
    }
}

void PluginHostComponent::cellDoubleClicked(int rowNumber, int /*columnId*/, const juce::MouseEvent&)
{
    if (rowNumber >= 0 && rowNumber < static_cast<int>(filteredPlugins.size()))
    {
        bool asInst = filteredPlugins[static_cast<size_t>(rowNumber)].isInstrument;
        loadSelectedPlugin(asInst);
    }
}

void PluginHostComponent::scanPlugins()
{
    juce::FileSearchPath searchPath;

    for (int i = 0; i < engine.getPluginFormatManager().getNumFormats(); ++i)
    {
        auto* format = engine.getPluginFormatManager().getFormat(i);
        if (format != nullptr)
        {
            auto defLocs = format->getDefaultLocationsToSearch();
            for (int p = 0; p < defLocs.getNumPaths(); ++p)
                searchPath.add(defLocs[p]);
        }
    }

    lblStatus.setText("Scanning plugin directories...", juce::dontSendNotification);

    engine.scanPluginDirectory(searchPath);
    updateFilteredList();
}

void PluginHostComponent::loadPluginFromFile()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Select VST3 or CLAP Plugin",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.vst3;*.clap"
    );

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                         [this, chooser](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file.exists())
        {
            juce::OwnedArray<juce::PluginDescription> found;
            for (int i = 0; i < engine.getPluginFormatManager().getNumFormats(); ++i)
            {
                auto* fmt = engine.getPluginFormatManager().getFormat(i);
                if (fmt != nullptr && fmt->fileMightContainThisPluginType(file.getFullPathName()))
                {
                    fmt->findAllTypesForFile(found, file.getFullPathName());
                }
            }

            for (const auto* d : found)
            {
                if (d != nullptr)
                    engine.getKnownPluginList().addType(*d);
            }

            updateFilteredList();
        }
    });
}

void PluginHostComponent::loadSelectedPlugin(bool asInstrument)
{
    int row = tablePlugins.getSelectedRow();
    if (row >= 0 && row < static_cast<int>(filteredPlugins.size()))
    {
        const auto& desc = filteredPlugins[static_cast<size_t>(row)];
        lblStatus.setText("Loading " + desc.name + "...", juce::dontSendNotification);

        engine.loadPluginAsync(desc, asInstrument, [this, desc](bool success, const juce::String& err) {
            if (success)
            {
                lblStatus.setText("Successfully loaded " + desc.name, juce::dontSendNotification);
                updateSlotDisplays();
            }
            else
            {
                lblStatus.setText("Failed to load " + desc.name + ": " + err, juce::dontSendNotification);
            }
        });
    }
}

void PluginHostComponent::openPluginEditor(bool forInstrument)
{
    auto* plugin = forInstrument ? engine.getActiveInstrumentPlugin() : engine.getActiveEffectPlugin();
    if (plugin == nullptr)
        return;

    auto* editor = plugin->createEditorIfNeeded();
    if (editor == nullptr)
        return;

    auto window = std::make_unique<juce::DocumentWindow>(
        plugin->getName() + " Editor",
        JazzLookAndFeel::Colors::backgroundDark,
        juce::DocumentWindow::allButtons
    );

    window->setContentOwned(editor, true);
    window->setResizable(true, true);
    window->setUsingNativeTitleBar(true);
    window->centreWithSize(editor->getWidth() > 100 ? editor->getWidth() : 600,
                           editor->getHeight() > 100 ? editor->getHeight() : 450);
    window->setVisible(true);

    if (forInstrument)
        instWindow = std::move(window);
    else
        fxWindow = std::move(window);
}

void PluginHostComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.fillAll(JazzLookAndFeel::Colors::backgroundDark);

    // Header strip
    auto topArea = bounds.removeFromTop(44);
    g.setColour(JazzLookAndFeel::Colors::panelHeader);
    g.fillRect(topArea);

    g.setColour(JazzLookAndFeel::Colors::neonCyan);
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    g.drawText("VST3 & CLAP AUDIO PLUGIN HOST", 16, 0, 320, 44, juce::Justification::centredLeft);
}

void PluginHostComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(44);

    // Bottom Slot Rack (140px)
    auto slotsArea = bounds.removeFromBottom(140).reduced(12, 6);
    int slotW = (slotsArea.getWidth() - 16) / 2;

    auto instArea = slotsArea.removeFromLeft(slotW);
    grpInstrument.setBounds(instArea);
    lblInstName.setBounds(instArea.getX() + 12, instArea.getY() + 24, instArea.getWidth() - 24, 22);
    btnInstBypass.setBounds(instArea.getX() + 12, instArea.getY() + 54, 80, 24);
    btnInstOpenUI.setBounds(instArea.getX() + 100, instArea.getY() + 54, 80, 24);
    btnInstUnload.setBounds(instArea.getX() + 188, instArea.getY() + 54, 80, 24);

    slotsArea.removeFromLeft(16);
    auto fxArea = slotsArea;
    grpEffect.setBounds(fxArea);
    lblFxName.setBounds(fxArea.getX() + 12, fxArea.getY() + 24, fxArea.getWidth() - 24, 22);
    btnFxBypass.setBounds(fxArea.getX() + 12, fxArea.getY() + 54, 80, 24);
    btnFxOpenUI.setBounds(fxArea.getX() + 100, fxArea.getY() + 54, 80, 24);
    btnFxUnload.setBounds(fxArea.getX() + 188, fxArea.getY() + 54, 80, 24);

    // Filter bar (36px)
    auto filterArea = bounds.removeFromTop(36).reduced(12, 4);
    btnFilterAll.setBounds(filterArea.getX(), filterArea.getY(), 50, 26);
    btnFilterVst3.setBounds(filterArea.getX() + 54, filterArea.getY(), 55, 26);
    btnFilterClap.setBounds(filterArea.getX() + 113, filterArea.getY(), 55, 26);

    txtSearch.setBounds(filterArea.getX() + 176, filterArea.getY(), 300, 26);

    btnScan.setBounds(filterArea.getRight() - 250, filterArea.getY(), 110, 26);
    btnLoadFile.setBounds(filterArea.getRight() - 132, filterArea.getY(), 110, 26);

    // Status / Action Strip below table (30px)
    auto actionArea = bounds.removeFromBottom(34).reduced(12, 4);
    lblStatus.setBounds(actionArea.getX(), actionArea.getY(), actionArea.getWidth() - 320, 26);
    btnLoadAsInst.setBounds(actionArea.getRight() - 310, actionArea.getY(), 150, 26);
    btnLoadAsFx.setBounds(actionArea.getRight() - 152, actionArea.getY(), 150, 26);

    // Center Table
    tablePlugins.setBounds(bounds.reduced(12, 4));
}
