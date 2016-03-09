#include "build.h"
#include "ngdp.h"
#include "app.h"
#include "program.h"
#include "tags.h"

enum {
  ID_BUILD = 1000,
};

BuildPage::BuildPage(Wizard* wizard)
  : Page(wizard)
{
  build_ = new ComboFrame(this, ID_BUILD);
  build_->addString("Loading CDN config...", -1);
  build_->setCurSel(0);
  build_->setPoint(PT_TOPLEFT, 120, 0);
  build_->setPoint(PT_RIGHT, 0, 0);
  build_->disable();
  ComboBox_SetCueBannerText(build_->getHandle(), L"Select one");
  StaticFrame::addTip(build_, "Select build");

  tags_ = new EditFrame(this, 0, ES_AUTOHSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY);
  tags_->setPoint(PT_TOP, build_, PT_BOTTOM, 0, 6);
  tags_->setPoint(PT_LEFT, 0, 0);
  tags_->setPoint(PT_BOTTOMRIGHT, 0, 0);
  tags_->hide();
}

void BuildPage::init() {
  wizard_->enableNext(false);

  auto& data = wizard_->app()->data();
  auto ngdp = data.ngdp();
  if (ngdp && ngdp->version() && ngdp->version()->cdn == data.builds_loaded && data.builds.size() == data.build_configs.size()) {
    build_->reset();
    for (std::string const& build : data.builds) {
      std::string name = data.build_configs[build]["build-name"];
      if (build == ngdp->version()->build) {
        name += " (current)";
      }
      build_->addString(name);
      if (build == data.selected_build) {
        build_->setCurSel(build_->getCount() - 1);
      }
    }
    build_->enable();
    onMessage(WM_COMMAND, MAKELONG(ID_BUILD, CBN_SELCHANGE), 0);
  } else {
    wizard_->app()->data().loadBuilds();
  }
}

LRESULT BuildPage::onMessage(uint32 message, WPARAM wParam, LPARAM lParam) {
  auto& data = wizard_->app()->data();
  switch (message) {
  case WM_COMMAND:
    if (LOWORD(wParam) == ID_BUILD && HIWORD(wParam) == CBN_SELCHANGE) {
      int index = build_->getCurSel();
      if (index >= 0 && index < data.builds.size() && data.build_configs.count(data.builds[index])) {
        static const std::string skipTags = "download|encoding|encoding-size|install|patch|patch-config|root";
        std::string text;
        auto const& build = data.build_configs[data.builds[index]];
        data.selected_build = data.builds[index];
        for (auto& kv : build) {
          if (skipTags.find(kv.first) != std::string::npos) continue;
          text.append(kv.first);
          text.append(" = ");
          text.append(kv.second);
          text.append("\r\n");
        }
        tags_->setText(text);
        tags_->show();
        wizard_->enableNext(true);
      } else {
        data.selected_build.clear();
        tags_->hide();
        wizard_->enableNext(false);
      }
    }
    return 0;
  case WM_TASKDONE:
    if (lParam == -1) {
      build_->reset();
      build_->addString("Failed to load CDN config", -1);
      build_->setCurSel(0);
      build_->disable();
    } else if (lParam == 0) {
      build_->reset();
      for (std::string const& build : data.builds) {
        build_->addString("Loading...", -1);
      }
      build_->enable();
    } else if (lParam != ProgramData::LOADING) {
      build_->delString(lParam - 1);
      std::string build = data.builds[lParam - 1];
      std::string name = data.build_configs[build]["build-name"];
      if (build == data.ngdp()->version()->build) {
        name += " (current)";
      }
      SendMessage(build_->getHandle(), CB_INSERTSTRING, lParam - 1, (LPARAM) name.c_str());
    }
    return 0;
  default:
    return M_UNHANDLED;
  }
}

Page* BuildPage::getPrev() {
  return new ProgramPage(wizard_);
}
Page* BuildPage::getNext() {
  auto& data = wizard_->app()->data();
  if (data.build_configs.count(data.selected_build)) {
    return new TagsPage(wizard_);
  } else {
    return nullptr;
  }
}
