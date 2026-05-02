// Copyright (C) 2026 - zsliu98
// This file is part of ZLSplitter
//
// ZLSplitter is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License Version 3 as published by the Free Software Foundation.
//
// ZLSplitter is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License along with ZLSplitter. If not, see <https://www.gnu.org/licenses/>.

#include "main_panel.hpp"

namespace zlpanel {
    MainPanel::MainPanel(PluginProcessor& p, zlgui::UIBase& base,
                         const multilingual::TooltipLanguage language) :
        base_(base),
        tooltip_helper_(language),
        curve_panel_(p, base_, tooltip_helper_),
        control_panel_(p, base_, tooltip_helper_),
        top_panel_(p, base_, tooltip_helper_),
        analyzer_setting_panel_(p, base),
        ui_setting_panel_(p, base_),
        refresh_handler_(zlstate::PTargetRefreshSpeed::kRates[base_.getRefreshRateID()]) {
        juce::ignoreUnused(base_);
        addAndMakeVisible(curve_panel_);
        addAndMakeVisible(control_panel_);
        addAndMakeVisible(top_panel_);
        addChildComponent(analyzer_setting_panel_);
        addChildComponent(ui_setting_panel_);

        // Setup fixed tooltip label at bottom-left
        tooltip_label_ = std::make_unique<juce::Label>();
        tooltip_label_->setInterceptsMouseClicks(false, false);
        tooltip_label_->setJustificationType(juce::Justification::centredLeft);
        tooltip_label_->setMinimumHorizontalScale(1.0f);
        tooltip_label_->setBorderSize(juce::BorderSize<int>(0));
        tooltip_label_->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible(tooltip_label_.get());

        base_.getPanelValueTree().addListener(this);

        startTimerHz(10);
    }

    MainPanel::~MainPanel() {
        base_.getPanelValueTree().removeListener(this);
        stopTimer();
    }

    void MainPanel::resized() {
        auto bound = getLocalBounds();

        {
            const auto height = static_cast<float>(bound.getHeight());
            const auto width = static_cast<float>(bound.getWidth());
            if (height < width * 0.575f) {
                bound.setHeight(static_cast<int>(std::ceil(width * .575f)));
            } else if (height > width * 1.f) {
                bound.setWidth(static_cast<int>(std::ceil(height * 1.f)));
            }
        }

        const auto max_font_size = static_cast<float>(bound.getWidth()) * kFontSizeOverWidth;
        const auto min_font_size = max_font_size * .25f;
        const auto font_size = base_.getFontMode() == 0
            ? max_font_size * base_.getFontScale()
            : std::clamp(base_.getStaticFontSize(), min_font_size, max_font_size);
        base_.setFontSize(font_size);

        ui_setting_panel_.setBounds(bound);

        top_panel_.setBounds(bound.removeFromTop(top_panel_.getIdealHeight()));
        curve_panel_.setBounds(bound);
        control_panel_.setBounds(bound.removeFromLeft(control_panel_.getIdealWidth()));
        analyzer_setting_panel_.setBounds(bound.removeFromRight(analyzer_setting_panel_.getIdealWidth()));

        // Position tooltip label at bottom-left
        if (tooltip_label_ != nullptr) {
            const auto padding = static_cast<int>(std::round(base_.getFontSize() * 0.5f));
            const auto labelHeight = static_cast<int>(std::ceil(base_.getFontSize() * 2.0f));
            const auto labelWidth = static_cast<int>(std::ceil(static_cast<float>(getWidth()) * 0.45f));
            tooltip_label_->setBounds(padding, getHeight() - labelHeight - padding,
                                      labelWidth, labelHeight);
            tooltip_label_->setFont(juce::FontOptions(base_.getFontSize()));
        }
    }

    void MainPanel::repaintCallBack(const double time_stamp) {
        // Update fixed tooltip every 50ms
        if (time_stamp - last_tooltip_update_ > 0.05) {
            last_tooltip_update_ = time_stamp;
            updateTooltip();
        }

        if (refresh_handler_.tick(time_stamp)) {
            if (time_stamp - previous_time_stamp_ > 0.1) {
                previous_time_stamp_ = time_stamp;
                curve_panel_.repaintCallBackSlow();
                control_panel_.repaintCallBackSlow();
                analyzer_setting_panel_.repaintCallBackSlow();
                top_panel_.repaintCallBackSlow();
            }

            curve_panel_.repaintCallBack(time_stamp);

            const auto c_refresh_rate = refresh_handler_.getActualRefreshRate();
            if (std::abs(c_refresh_rate - refresh_rate_) > 1e-3) {
                refresh_rate_ = c_refresh_rate;
                curve_panel_.setRefreshRate(refresh_rate_);
            }
        }
    }

    void MainPanel::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property) {
        if (base_.isPanelIdentifier(zlgui::PanelSettingIdx::kUISettingPanel, property)) {
            const auto ui_setting_visibility = static_cast<bool>(base_.getPanelProperty(
                zlgui::PanelSettingIdx::kUISettingPanel));
            curve_panel_.setVisible(!ui_setting_visibility);
            ui_setting_panel_.setVisible(ui_setting_visibility);
        }
    }

    void MainPanel::timerCallback() {
        if (juce::Process::isForegroundProcess()) {
            if (getCurrentlyFocusedComponent() != this) {
                grabKeyboardFocus();
            }
            stopTimer();
        }
    }

    void MainPanel::updateTooltip() {
        const auto mousePos = juce::Desktop::getInstance().getMainMouseSource().getScreenPosition();
        const auto localPos = getLocalPoint(nullptr, mousePos.toInt());

        if (!reallyContains(localPos, false)) {
            tooltip_label_->setVisible(false);
            return;
        }

        auto* comp = getComponentAt(localPos);
        if (comp == nullptr || comp == tooltip_label_.get()) {
            tooltip_label_->setVisible(false);
            return;
        }

        const auto tooltip = comp->getTooltip();
        if (tooltip.isEmpty()) {
            tooltip_label_->setVisible(false);
            return;
        }

        tooltip_label_->setText(tooltip, juce::dontSendNotification);
        tooltip_label_->setVisible(true);

        // Update colours to match current theme
        tooltip_label_->setColour(juce::Label::textColourId, base_.getTextColour());
        tooltip_label_->setColour(juce::Label::backgroundColourId,
                                  base_.getBackgroundColour().withAlpha(0.875f));
    }
}
