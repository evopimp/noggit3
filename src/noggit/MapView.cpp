// This file is part of Noggit3, licensed under GNU General Public License (version 3).

#undef _UNICODE

#include <noggit/Brush.h> // brush
#include <noggit/ConfigFile.h>
#include <noggit/DBC.h>
#include <noggit/Environment.h>
#include <noggit/Environment.h>
#include <noggit/FreeType.h> // freetype::
#include <noggit/Log.h>
#include <noggit/MapChunk.h>
#include <noggit/MapView.h>
#include <noggit/Misc.h>
#include <noggit/ModelManager.h> // ModelManager
#include <noggit/Project.h>
#include <noggit/Settings.h>
#include <noggit/TextureManager.h> // TextureManager, Texture
#include <noggit/WMOInstance.h> // WMOInstance
#include <noggit/World.h>
#include <noggit/application.h> // app.getStates(), gPop, app.getArial14(), morpheus40, arial...
#include <noggit/map_index.hpp>
#include <noggit/ui/CapsWarning.h>
#include <noggit/ui/CheckBox.h> // UICheckBox
#include <noggit/ui/CursorSwitcher.h> // UICursorSwitcher
#include <noggit/ui/DetailInfos.h> // detailInfos
#include <noggit/ui/FlattenTool.hpp>
#include <noggit/ui/Gradient.h> // UIGradient
#include <noggit/ui/HelperModels.h>
#include <noggit/ui/MapViewGUI.h> // UIMapViewGUI
#include <noggit/ui/MenuBar.h> // UIMenuBar, menu items, ..
#include <noggit/ui/MinimapWindow.h> // UIMinimapWindow
#include <noggit/ui/ModelImport.h>
#include <noggit/ui/ObjectEditor.h>
#include <noggit/ui/RotationEditor.h>
#include <noggit/ui/shader_tool.hpp>
#include <noggit/ui/Slider.h> // UISlider
#include <noggit/ui/StatusBar.h> // statusBar
#include <noggit/ui/terrain_tool.hpp>
#include <noggit/ui/Text.h> // UIText
#include <noggit/ui/Texture.h> // textureUI
#include <noggit/ui/TexturePicker.h>
#include <noggit/ui/TextureSwitcher.h>
#include <noggit/ui/TexturingGUI.h>
#include <noggit/ui/texturing_tool.hpp>
#include <noggit/ui/ToggleGroup.h> // UIToggleGroup
#include <noggit/ui/Toolbar.h> // UIToolbar
#include <noggit/ui/ToolbarIcon.h> // ToolbarIcon
#include <noggit/ui/Water.h>
#include <noggit/ui/WaterSaveWarning.h>
#include <noggit/ui/WaterTypeBrowser.h>
#include <noggit/ui/ZoneIDBrowser.h>
#include <opengl/matrix.hpp>
#include <opengl/scoped.hpp>

#include "revision.h"

#include <boost/filesystem.hpp>

#include <QtCore/QTimer>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

static const float XSENS = 15.0f;
static const float YSENS = 15.0f;
static const float SPEED = 200.6f;

void MapView::set_editing_mode (editing_mode mode)
{
  mainGui->guiWaterTypeSelector->hide();
  mainGui->terrainTool->hide();
  mainGui->flattenTool->hide();
  mainGui->texturingTool->hide();
  mainGui->shaderTool->hide();
  mainGui->guiWater->hide();
  mainGui->TexturePicker->hide();
  mainGui->objectEditor->hide();
  mainGui->objectEditor->modelImport->hide();
  mainGui->rotationEditor->hide();
  mainGui->ZoneIDBrowser->hide();

  if (!mainGui || !mainGui->TexturePalette)
    return;
  mainGui->TexturePalette->hide();
  // fetch old win position
  switch (terrainMode)
  {
  case editing_mode::ground:
    tool_settings_x = mainGui->terrainTool->x();
    tool_settings_y = mainGui->terrainTool->y();
    break;
  case editing_mode::flatten_blur:
    tool_settings_x = mainGui->flattenTool->x();
    tool_settings_y = mainGui->flattenTool->y();
    break;
  case editing_mode::paint:
    tool_settings_x = mainGui->texturingTool->x();
    tool_settings_y = mainGui->texturingTool->y();
    break;
  case editing_mode::areaid:
    tool_settings_x = mainGui->ZoneIDBrowser->x() + 230;
    tool_settings_y = mainGui->ZoneIDBrowser->y();
    break;
  case editing_mode::water:
    tool_settings_x = mainGui->guiWater->x();
    tool_settings_y = mainGui->guiWater->y();
    break;
  case editing_mode::mccv:
    tool_settings_x = mainGui->shaderTool->x();
    tool_settings_y = mainGui->shaderTool->y();
    break;
  }
  // set new win pos and make visible
  switch (mode)
  {
  case editing_mode::ground:
    mainGui->terrainTool->move (tool_settings_x, tool_settings_y);
    mainGui->terrainTool->show();
    break;
  case editing_mode::flatten_blur:
    mainGui->flattenTool->move(tool_settings_x, tool_settings_y);
    mainGui->flattenTool->show();
    break;
  case editing_mode::paint:
    mainGui->texturingTool->x(tool_settings_x);
    mainGui->texturingTool->y(tool_settings_y);
    mainGui->texturingTool->show();
    break;
  case editing_mode::areaid:
    mainGui->ZoneIDBrowser->move(mainGui->ZoneIDBrowser->x(), tool_settings_y);
    mainGui->ZoneIDBrowser->show();
    break;
  case editing_mode::water:
    mainGui->guiWater->x(tool_settings_x);
    mainGui->guiWater->y(tool_settings_y);
    mainGui->guiWater->show();
    break;
  case editing_mode::mccv:
    mainGui->shaderTool->x(tool_settings_x);
    mainGui->shaderTool->y(tool_settings_y);
    mainGui->shaderTool->show();
    break;
  case editing_mode::object:
    mainGui->objectEditor->show();
    mainGui->rotationEditor->move (mainGui->objectEditor->x() - mainGui->rotationEditor->width() - 10.0f, tool_settings_y);
  }

  terrainMode = mode;
  mainGui->guiToolbar->IconSelect (mode);
}

void MapView::ResetSelectedObjectRotation()
{
  if (gWorld->IsSelection(eEntry_WMO))
  {
    WMOInstance* wmo = boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection());
    gWorld->updateTilesWMO(wmo);
    wmo->resetDirection();
    gWorld->updateTilesWMO(wmo);
  }
  else if (gWorld->IsSelection(eEntry_Model))
  {
    ModelInstance* m2 = boost::get<selected_model_type> (*gWorld->GetCurrentSelection());
    gWorld->updateTilesModel(m2);
    m2->resetDirection();
    m2->recalcExtents();
    gWorld->updateTilesModel(m2);
  }
}

void MapView::SnapSelectedObjectToGround()
{
  if (gWorld->IsSelection(eEntry_WMO))
  {
    WMOInstance* wmo = boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection());
    math::vector_3d t = math::vector_3d(wmo->pos.x, wmo->pos.z, 0);
    gWorld->GetVertex(wmo->pos.x, wmo->pos.z, &t);
    wmo->pos.y = t.y;
    gWorld->updateTilesWMO(wmo);
  }
  else if (gWorld->IsSelection(eEntry_Model))
  {
    ModelInstance* m2 = boost::get<selected_model_type> (*gWorld->GetCurrentSelection());
    math::vector_3d t = math::vector_3d(m2->pos.x, m2->pos.z, 0);
    gWorld->GetVertex(m2->pos.x, m2->pos.z, &t);
    m2->pos.y = t.y;
    gWorld->updateTilesModel(m2);
  }
}


void MapView::DeleteSelectedObject()
{
  if (gWorld->IsSelection(eEntry_WMO))
  {
    gWorld->deleteWMOInstance(boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->mUniqueID);
  }
  else if (gWorld->IsSelection(eEntry_Model))
  {
    gWorld->deleteModelInstance(boost::get<selected_model_type> (*gWorld->GetCurrentSelection())->d1);
  }
}

void MapView::insert_last_m2_from_wmv()
{
  //! \todo Beautify.

  // Test if there is an selection
  if (!gWorld->HasSelection())
    return;

  std::string importFile (Settings::getInstance()->wmvLogFile);

  std::string lastModel;

  size_t foundString;
  std::string line;
  std::string findThis;
  std::ifstream fileReader(importFile.c_str());
  if (fileReader.is_open())
  {
    while (!fileReader.eof())
    {
      getline(fileReader, line);
      std::transform(line.begin(), line.end(), line.begin(), ::tolower);

      if (line.find(".m2") != std::string::npos || line.find(".mdx") != std::string::npos)
      {
        // M2 inside line
        // is it the modelviewer log then cut the log messages out
        findThis = "loading model: ";
        foundString = line.find(findThis);
        if (foundString != std::string::npos)
        {
          // cut path
          line = line.substr(foundString + findThis.size());
        }
        else
        {
          // invalid line
          continue;
        }
      }
      // swap mdx to m2
      size_t found = line.rfind(".mdx");
      if (found != std::string::npos)
      {
        line.replace(found, 4, ".m2");
      }

      lastModel = line.substr(0, line.find(".m2") + 3);
    }
  }
  else
  {
    // file not exist, no rights ore other error
    LogError << importFile << std::endl;
  }

  math::vector_3d selectionPosition;
  switch (gWorld->GetCurrentSelection()->which())
  {
  case eEntry_Model:
    selectionPosition = boost::get<selected_model_type> (*gWorld->GetCurrentSelection())->pos;
    break;
  case eEntry_WMO:
    selectionPosition = boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->pos;
    break;
  case eEntry_MapChunk:
    selectionPosition = boost::get<selected_chunk_type> (*gWorld->GetCurrentSelection()).position;
    break;
  }

  if (lastModel != "")
  {
    if (!MPQFile::exists(lastModel))
    {
      LogError << "Failed adding " << lastModel << ". It was not in any MPQ." << std::endl;
    }
    else
    {
      gWorld->addM2(lastModel, selectionPosition, false);
    }
  }
}

void MapView::insert_last_wmo_from_wmv()
{
  //! \todo Beautify.

  if (!gWorld->HasSelection())
    return;

  std::string importFile (Settings::getInstance()->wmvLogFile);

  std::string lastWMO;

  size_t foundString;
  std::string line;
  std::string findThis;
  std::ifstream fileReader(importFile.c_str());
  if (fileReader.is_open())
  {
    while (!fileReader.eof())
    {
      getline(fileReader, line);
      std::transform(line.begin(), line.end(), line.begin(), ::tolower);

      if (line.find(".wmo") != std::string::npos)
      {
        // WMO inside line
        findThis = "loading wmo ";
        foundString = line.find(findThis);
        // is it the modelviewer log then cut the log messages out
        if (foundString != std::string::npos)
        {
          // cut path
          line = line.substr(foundString + findThis.size());
        }
        else
        {
          // invalid line
          continue;
        }

        lastWMO = line.substr(0, line.find(".wmo") + 4);
      }
    }
  }
  else
  {
    // file not exist, no rights ore other error
    LogError << importFile << std::endl;
  }


  math::vector_3d selectionPosition;
  switch (gWorld->GetCurrentSelection()->which())
  {
  case eEntry_Model:
    selectionPosition = boost::get<selected_model_type> (*gWorld->GetCurrentSelection())->pos;
    break;
  case eEntry_WMO:
    selectionPosition = boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->pos;
    break;
  case eEntry_MapChunk:
    selectionPosition = boost::get<selected_chunk_type> (*gWorld->GetCurrentSelection()).position;
    break;
  }

  if (lastWMO != "")
  {
    if (!MPQFile::exists(lastWMO))
    {
      LogError << "Failed adding " << lastWMO << ". It was not in any MPQ." << std::endl;
    }
    else
    {
      gWorld->addWMO(lastWMO, selectionPosition, false);
    }
  }
  //! \todo Memoryleak: These models will never get deleted.
}



void MapView::changeZoneIDValue (int set)
{
  _selected_area_id = set;
}


void MapView::createGUI()
{
  // create main gui object that holds all other gui elements for access ( in the future ;) )
  mainGui = new UIMapViewGUI(this, &gWorld->camera);
  mainGui->guiCurrentTexture->current_texture->setClickFunc ([this] { mainGui->TexturePalette->toggleVisibility(); });

  mainGui->ZoneIDBrowser->setMapID(gWorld->getMapID());
  mainGui->ZoneIDBrowser->setChangeFunc([this] (int id){ changeZoneIDValue (id); });
  tool_settings_x = video.xres() - 186;
  tool_settings_y = 38;

  mainGui->terrainTool->storeCursorPos (&_cursor_pos);

  mainGui->addChild(mainGui->TexturePalette = UITexturingGUI::createTexturePalette(mainGui));
  mainGui->TexturePalette->hide();
  mainGui->addChild(UITexturingGUI::createTilesetLoader());
  mainGui->addChild(UITexturingGUI::createTextureFilter());
  mainGui->addChild(MapChunkWindow = UITexturingGUI::createMapChunkWindow());
  MapChunkWindow->hide();

  // create the menu
  UIMenuBar * mbar = new UIMenuBar();

  mbar->AddMenu("File");
  mbar->AddMenu("Edit");
  mbar->AddMenu("View");
  mbar->AddMenu("Assist");
  mbar->AddMenu("Help");

  mbar->GetMenu( "File" )->AddMenuItemButton( "CTRL+SHIFT+S Save current", [this] { prompt_save_current(); });
  mbar->GetMenu("File")->AddMenuItemButton("CTRL+S Save", [] { gWorld->mapIndex.saveChanged(); });
  mbar->GetMenu("File")->AddMenuItemButton("CTRL+SHIFT+A Save all", [] { gWorld->mapIndex.saveall(); });
  addHotkey (Qt::Key_S, MOD_ctrl | MOD_shift, [this] { prompt_save_current(); });
  addHotkey (Qt::Key_A, MOD_ctrl | MOD_shift, [this] { gWorld->mapIndex.saveall(); });
  addHotkey (Qt::Key_S, MOD_ctrl, [this] { gWorld->mapIndex.saveChanged(); });
  addHotkey (Qt::Key_S, MOD_meta, [this] { gWorld->mapIndex.saveChanged(); });
  mbar->GetMenu( "File" )->AddMenuItemButton( "SHIFT+J Reload tile", [] { gWorld->mapIndex.reloadTile(tile_index(gWorld->camera)); });
  addHotkey (Qt::Key_J, MOD_shift, [] { gWorld->mapIndex.reloadTile(tile_index(gWorld->camera)); });
  mbar->GetMenu("File")->AddMenuItemSeperator(" ");
  mbar->GetMenu("File")->AddMenuItemButton("ESC Exit", [this] { prompt_exit(); });
  addHotkey (Qt::Key_Escape, MOD_none, [this] { prompt_exit(); });

  mbar->GetMenu("Edit")->AddMenuItemSeperator("selected object");
  mbar->GetMenu("Edit")->AddMenuItemButton("DEL delete", [this] { DeleteSelectedObject(); });
  addHotkey (Qt::Key_Delete, MOD_none, [this] { DeleteSelectedObject(); });
  mbar->GetMenu("Edit")->AddMenuItemButton("CTRL + R reset rotation", [this] { ResetSelectedObjectRotation(); });
  addHotkey (Qt::Key_R, MOD_ctrl, [this] { ResetSelectedObjectRotation(); });
  mbar->GetMenu("Edit")->AddMenuItemButton("PAGE DOWN set to ground", [this] { SnapSelectedObjectToGround(); });
  addHotkey (Qt::Key_PageDown, MOD_none, [this] { SnapSelectedObjectToGround(); });

  mbar->GetMenu("Edit")->AddMenuItemSeperator("Options");
  mbar->GetMenu("Edit")->AddMenuItemToggle("Auto select mode", &Settings::getInstance()->AutoSelectingMode, false);


  mbar->GetMenu("Assist")->AddMenuItemSeperator("Model");
  mbar->GetMenu("Assist")->AddMenuItemButton("Last M2 from MV", [this] { insert_last_m2_from_wmv(); });
  addHotkey (Qt::Key_V, MOD_shift, [this] { insert_last_m2_from_wmv(); });
  mbar->GetMenu("Assist")->AddMenuItemButton("Last WMO from MV", [this] { insert_last_wmo_from_wmv(); });
  addHotkey (Qt::Key_V, MOD_alt, [this] { insert_last_wmo_from_wmv(); });
  mbar->GetMenu("Assist")->AddMenuItemButton("Helper models", [this] { mainGui->HelperModels->show(); });
  mbar->GetMenu("Assist")->AddMenuItemSeperator("Current ADT");
  mbar->GetMenu("Assist")->AddMenuItemButton ( "Set Area ID"
                                             , [this]
                                               {
                                                 if (_selected_area_id != -1)
                                                 {
                                                   gWorld->setAreaID(gWorld->camera, _selected_area_id, true);
                                                 }
                                               }
                                             );
  mbar->GetMenu("Assist")->AddMenuItemButton ( "Clear height map"
                                             , []
                                               {
                                                 gWorld->clearHeight(gWorld->camera);
                                               }
                                             );

  mbar->GetMenu("Assist")->AddMenuItemButton ( "Clear texture"
                                             , [] { gWorld->setBaseTexture(gWorld->camera); }
                                             );
  mbar->GetMenu("Assist")->AddMenuItemButton ( "Clear models"
                                             , [] { gWorld->clearAllModelsOnADT(gWorld->camera); }
                                             );
  mbar->GetMenu("Assist")->AddMenuItemButton ( "Clear duplicate models"
                                             , [] { gWorld->delete_duplicate_model_and_wmo_instances(); }
                                             );
  mbar->GetMenu("Assist")->AddMenuItemSeperator("Loaded ADTs");
  mbar->GetMenu("Assist")->AddMenuItemButton("Fix gaps (all loaded ADTs)", [] { gWorld->fixAllGaps(); });

  mbar->GetMenu("Assist")->AddMenuItemSeperator("Global");
  mbar->GetMenu("Assist")->AddMenuItemButton("Map to big alpha", [] { gWorld->convert_alphamap(true); });
  mbar->GetMenu("Assist")->AddMenuItemButton("Map to old", [] { gWorld->convert_alphamap(false); });

  mbar->GetMenu("View")->AddMenuItemSeperator("Windows");
  mbar->GetMenu("View")->AddMenuItemToggle("Toolbar", mainGui->guiToolbar->hidden_evil(), true);

  mbar->GetMenu("View")->AddMenuItemToggle("Texture palette", mainGui->TexturePalette->hidden_evil(), true);
  mbar->GetMenu("View")->AddMenuItemButton("Cursor options", [this] { mainGui->showCursorSwitcher(); });
  mbar->GetMenu("View")->AddMenuItemSeperator("Toggle");
  mbar->GetMenu("View")->AddMenuItemToggle("F1 M2s", &_draw_models);
  addHotkey (Qt::Key_F1, MOD_none, [this] { _draw_models = !_draw_models; });
  mbar->GetMenu("View")->AddMenuItemToggle("F2 WMO doodadsets", &_draw_wmo_doodads);
  addHotkey (Qt::Key_F2, MOD_none, [this] { _draw_wmo_doodads = !_draw_wmo_doodads; });
  mbar->GetMenu("View")->AddMenuItemToggle("F3 Terrain", &_draw_terrain);
  addHotkey (Qt::Key_F3, MOD_none, [this] { _draw_terrain = !_draw_terrain; });
  mbar->GetMenu("View")->AddMenuItemToggle("F4 Water", &_draw_water);
  addHotkey (Qt::Key_F4, MOD_none, [this] { _draw_water = !_draw_water; });
  mbar->GetMenu("View")->AddMenuItemToggle("F6 WMOs", &_draw_wmo);
  addHotkey (Qt::Key_F6, MOD_none, [this] { _draw_wmo = !_draw_wmo; });
  mbar->GetMenu("View")->AddMenuItemToggle("F7 Lines", &_draw_lines);
  addHotkey (Qt::Key_F7, MOD_none, [this] { _draw_lines = !_draw_lines; });
  mbar->GetMenu ("View")->AddMenuItemButton( "F8 Detail infos"
                                           , [this]
                                             {
                                               mainGui->guidetailInfos->toggle_visibility ();
                                             }
                                           );
  addHotkey (Qt::Key_F8, MOD_none, [this] { mainGui->guidetailInfos->toggle_visibility(); });
  mbar->GetMenu("View")->AddMenuItemToggle("F9 Map contour infos", &_draw_contour);
  addHotkey (Qt::Key_F9, MOD_none, [this] { _draw_contour = !_draw_contour; });
  mbar->GetMenu("View")->AddMenuItemToggle("F10 Wireframe", &_draw_wireframe);
  addHotkey(Qt::Key_F10, MOD_none, [this] { _draw_wireframe = !_draw_wireframe; });
  mbar->GetMenu("View")->AddMenuItemToggle("F11 Toggle Animation", &_draw_model_animations);
  addHotkey (Qt::Key_F11, MOD_none, [this] { _draw_model_animations = !_draw_model_animations; });
  mbar->GetMenu("View")->AddMenuItemToggle("F12 Fog", &gWorld->drawfog);
  addHotkey(Qt::Key_F12, MOD_none, [] { gWorld->drawfog = !gWorld->drawfog; });
  mbar->GetMenu("View")->AddMenuItemToggle("Flight Bounds", &_draw_mfbo);
  mbar->GetMenu("View")->AddMenuItemToggle("SHIFT+F7 Hole lines always on", &_draw_hole_lines, false);
  addHotkey (Qt::Key_F7, MOD_shift, [this] { _draw_hole_lines = !_draw_hole_lines; });
  mbar->GetMenu("View")->AddMenuItemToggle("Models with box", &_draw_models_with_box);

  mbar->GetMenu("Help")->AddMenuItemButton("H Key Bindings", [this] { mainGui->showHelp(); });
#if defined(_WIN32) || defined(WIN32)
  mbar->GetMenu("Help")->AddMenuItemButton ( "Manual online"
                                           , []
                                             {
                                               ShellExecute ( nullptr
                                                            , "open"
                                                            , "http://modcraft.superparanoid.de/wiki/index.php5?title=Noggit_user_manual"
                                                            , nullptr
                                                            , nullptr
                                                            , SW_SHOWNORMAL
                                                            );
                                             }
                                           );
  mbar->GetMenu("Help")->AddMenuItemButton ( "Homepage"
                                           , []
                                             {
                                               ShellExecute ( nullptr
                                                            , "open"
                                                            , "http://modcraft.superparanoid.de"
                                                            , nullptr
                                                            , nullptr
                                                            , SW_SHOWNORMAL
                                                            );
                                             }
                                           );
#endif

  mainGui->addChild(mbar);

  addHotkey (Qt::Key_M, MOD_none, [this] { mainGui->minimapWindow->toggleVisibility(); });

  addHotkey ( Qt::Key_F1
            , MOD_shift
            , [this]
              {
                if (alloff)
                {
                  alloff_models = _draw_models;
                  alloff_doodads = _draw_wmo_doodads;
                  alloff_contour = _draw_contour;
                  alloff_wmo = _draw_wmo;
                  alloff_fog = gWorld->drawfog;
                  alloff_terrain = _draw_terrain;

                  _draw_models = false;
                  _draw_wmo_doodads = false;
                  _draw_contour = true;
                  _draw_wmo = false;
                  _draw_terrain = true;
                  gWorld->drawfog = false;
                }
                else
                {
                  _draw_models = alloff_models;
                  _draw_wmo_doodads = alloff_doodads;
                  _draw_contour = alloff_contour;
                  _draw_wmo = alloff_wmo;
                  _draw_terrain = alloff_terrain;
                  gWorld->drawfog = alloff_fog;
                }
                alloff = !alloff;
              }
            );

  addHotkey ( Qt::Key_F5
            , MOD_none
            , [this]
              {
                std::ofstream f("bookmarks.txt", std::ios_base::app);
                f << gWorld->getMapID() << " " << gWorld->camera.x << " " << gWorld->camera.y << " " << gWorld->camera.z << " " << _camera_ah << " " << _camera_av << " " << gWorld->getAreaID (gWorld->camera) << std::endl;
              }
            );

  addHotkey (Qt::Key_N, MOD_none, [this] { mTimespeed += 90.0f; });
  addHotkey (Qt::Key_B, MOD_none, [this] { mTimespeed = std::max (0.0f, mTimespeed - 90.0f); });
  addHotkey (Qt::Key_J, MOD_none, [this] { mTimespeed = 0.0f; });

  addHotkey (Qt::Key_Tab, MOD_none, [this] { _GUIDisplayingEnabled = !_GUIDisplayingEnabled; });

  addHotkey ( Qt::Key_C
            , MOD_ctrl
            , [this]
              {
                mainGui->objectEditor->copy (*gWorld->GetCurrentSelection());
              }
            , [this]
              {
                return !!gWorld->GetCurrentSelection();
              }
            );

  addHotkey ( Qt::Key_C
            , MOD_alt | MOD_ctrl
            , [this]
              {
                mainGui->toggleCursorSwitcher();
              }
            );

  addHotkey ( Qt::Key_C
            , MOD_none
            , [this]
              {
                mainGui->objectEditor->copy(*gWorld->GetCurrentSelection());
              }
            , [this] { return terrainMode == editing_mode::object; }
            );

  addHotkey ( Qt::Key_C
            , MOD_shift
            , [this]
              {
                cursor_type = ++cursor_type % 4;
              }
            , [this] { return terrainMode != editing_mode::object; }
            );

  addHotkey (Qt::Key_V, MOD_ctrl, [this] { mainGui->objectEditor->pasteObject (_cursor_pos, gWorld->camera); });
  addHotkey ( Qt::Key_V
            , MOD_none
            , [this] { mainGui->objectEditor->pasteObject (_cursor_pos, gWorld->camera); }
            , [this] { return terrainMode == editing_mode::object; }
            );

  addHotkey ( Qt::Key_C
            , MOD_none
            , [] { gWorld->clearVertexSelection(); }
            , [this] { return terrainMode == editing_mode::ground; }
            );

  addHotkey ( Qt::Key_X
            , MOD_none
            , [this] { mainGui->TexturePalette->toggleVisibility(); }
            , [this] { return terrainMode == editing_mode::paint; }
            );

  addHotkey (Qt::Key_F4, MOD_shift, [] { Settings::getInstance()->AutoSelectingMode = !Settings::getInstance()->AutoSelectingMode; });

  addHotkey (Qt::Key_X, MOD_ctrl, [this] { mainGui->guidetailInfos->toggle_visibility(); });

  addHotkey (Qt::Key_I, MOD_none, [this] { mousedir *= -1.f; });

  addHotkey (Qt::Key_O, MOD_none, [this] { movespd *= 0.5f; });
  addHotkey (Qt::Key_P, MOD_none, [this] { movespd *= 2.0f; });

  addHotkey (Qt::Key_P, MOD_shift | MOD_ctrl, [this] { Saving = true; });

  addHotkey (Qt::Key_R, MOD_none, [this] { _camera_ah += 180.f; });

  addHotkey ( Qt::Key_G
            , MOD_none
            , []
              {
                // write teleport cords to txt file
                std::ofstream f("ports.txt", std::ios_base::app);
                f << "Map: " << gAreaDB.getAreaName(gWorld->getAreaID (gWorld->camera)) << " on ADT " << std::floor(gWorld->camera.x / TILESIZE) << " " << std::floor(gWorld->camera.z / TILESIZE) << std::endl;
                f << "Trinity:" << std::endl << ".go " << (ZEROPOINT - gWorld->camera.z) << " " << (ZEROPOINT - gWorld->camera.x) << " " << gWorld->camera.y << " " << gWorld->getMapID() << std::endl;
                f << "ArcEmu:" << std::endl << ".worldport " << gWorld->getMapID() << " " << (ZEROPOINT - gWorld->camera.z) << " " << (ZEROPOINT - gWorld->camera.x) << " " << gWorld->camera.y << " " << std::endl << std::endl;
                f.close();
              }
            );

  addHotkey ( Qt::Key_Y
            , MOD_none
            , [this] { mainGui->terrainTool->nextType(); }
            , [this] { return terrainMode == editing_mode::ground; }
            );

  addHotkey ( Qt::Key_Y
            , MOD_none
            , [this] { mainGui->flattenTool->nextFlattenType(); }
            , [this] { return terrainMode == editing_mode::flatten_blur; }
            );

  addHotkey ( Qt::Key_U
            , MOD_none
            , [this]
              {
                if (mViewMode == eViewMode_2D)
                {
                  mViewMode = eViewMode_3D;
                  set_editing_mode (saveterrainMode);
                }
                else
                {
                  mViewMode = eViewMode_2D;
                  saveterrainMode = terrainMode;
                  set_editing_mode (editing_mode::paint);
                }
              }
            );

  addHotkey ( Qt::Key_T
            , MOD_none
            , [&]
              {
                //! \todo space as global modifier?
                if (_mod_space_down)
                {
                  mainGui->flattenTool->nextFlattenMode();
                }
                else
                {
                  mainGui->flattenTool->toggleFlattenAngle();
                }
              }
            , [&] { return terrainMode == editing_mode::flatten_blur; }
            );

  addHotkey ( Qt::Key_T
            , MOD_none
            , [&]
              {
                mainGui->texturingTool->toggle_spray();
              }
            , [&] { return terrainMode == editing_mode::paint; }
            );

  addHotkey ( Qt::Key_T
            , MOD_none
            , [&]
              {
                gWorld->setHoleADT (gWorld->camera, _mod_alt_down);
              }
            , [&] { return terrainMode == editing_mode::holes; }
            );

  addHotkey( Qt::Key_T
           , MOD_none
           , [&]
             {
               mainGui->guiWater->toggle_angled_mode();
             }
          , [&] { return terrainMode == editing_mode::water; }
          );


  addHotkey ( Qt::Key_T
            , MOD_none
            , [&]
              {
                mainGui->objectEditor->togglePasteMode();
              }
            , [&] { return terrainMode == editing_mode::object; }
            );

  addHotkey ( Qt::Key_H
            , MOD_none
            , [&]
              {
                mainGui->toggleHelp();
              }
            , [&] { return terrainMode != editing_mode::object; }
            );

  addHotkey ( Qt::Key_H
            , MOD_none
            , [&]
              {
                // toggle hidden models visibility
                if (_mod_space_down)
                {
                  _draw_hidden_models = !_draw_hidden_models;
                }
                else if (_mod_shift_down)
                {
                  _hidden_map_objects.clear();
                  _hidden_models.clear();
                }
                else
                {
                  // toggle selected model visibility
                  if (gWorld->HasSelection())
                  {
                    auto selection = gWorld->GetCurrentSelection();
                    if (selection->which() == eEntry_Model)
                    {
                      auto&& entity (boost::get<selected_model_type> (*selection)->model.get());
                      auto& hidden (_hidden_models);
                      if (hidden.count (entity))
                      {
                        hidden.erase (entity);
                      }
                      else
                      {
                        hidden.emplace (entity);
                      }
                    }
                    else if (selection->which() == eEntry_WMO)
                    {
                      auto&& entity (boost::get<selected_wmo_type> (*selection)->wmo.get());
                      auto& hidden (_hidden_map_objects);
                      if (hidden.count (entity))
                      {
                        hidden.erase (entity);
                      }
                      else
                      {
                        hidden.emplace (entity);
                      }
                    }
                  }
                }
              }
            , [&] { return terrainMode == editing_mode::object; }
            );

  addHotkey ( Qt::Key_F
            , MOD_none
            , [&]
              {
                if (_mod_space_down)
                {
                  mainGui->terrainTool->flattenVertices();
                }
              }
            , [&] { return terrainMode == editing_mode::ground; }
            );
  addHotkey ( Qt::Key_F
            , MOD_none
            , [&]
              {
                if (_mod_space_down)
                {
                  mainGui->flattenTool->toggleFlattenLock();
                }
                else
                {
                  mainGui->flattenTool->lockPos (_cursor_pos);
                }
              }
            , [&] { return terrainMode == editing_mode::flatten_blur; }
            );
  addHotkey( Qt::Key_F
            , MOD_none
            , [&]
              {
                if (_mod_space_down)
                {
                  mainGui->guiWater->toggle_lock();
                }
                else
                {
                  mainGui->guiWater->lockPos(_cursor_pos);
                }
              }
          , [&] { return terrainMode == editing_mode::water; }
          );
  addHotkey ( Qt::Key_F
            , MOD_none
            , [&]
              {
                if (gWorld->HasSelection())
                {
                  auto selection = gWorld->GetCurrentSelection();

                  if (selection->which() == eEntry_Model)
                  {
                    gWorld->updateTilesModel(boost::get<selected_model_type> (*selection));
                    boost::get<selected_model_type> (*selection)->pos = _cursor_pos;
                    boost::get<selected_model_type> (*selection)->recalcExtents();
                    gWorld->updateTilesModel(boost::get<selected_model_type> (*selection));
                  }
                  else if (selection->which() == eEntry_WMO)
                  {
                    gWorld->updateTilesWMO(boost::get<selected_wmo_type> (*selection));
                    boost::get<selected_wmo_type> (*selection)->pos = _cursor_pos;
                    boost::get<selected_wmo_type> (*selection)->recalcExtents();
                    gWorld->updateTilesWMO(boost::get<selected_wmo_type> (*selection));
                  }
                }
              }
            , [&] { return terrainMode == editing_mode::object; }
            );

  addHotkey (Qt::Key_Plus, MOD_alt, [this] { mainGui->terrainTool->changeRadius(0.01f); }, [this] { return terrainMode == editing_mode::ground; });

  addHotkey (Qt::Key_Plus, MOD_alt, [this] { mainGui->flattenTool->changeRadius(0.01f); }, [this] { return terrainMode == editing_mode::flatten_blur; });

  addHotkey ( Qt::Key_Plus
            , MOD_alt
            , [&]
              {
                mainGui->texturingTool->change_radius(0.1f);
              }
            , [this] { return terrainMode == editing_mode::paint; }
            );

  addHotkey (Qt::Key_Plus, MOD_shift, [] { gWorld->fogdistance += 60.0f; });


  addHotkey (Qt::Key_Minus, MOD_alt, [this] { mainGui->terrainTool->changeRadius(-0.01f); }, [this] { return terrainMode == editing_mode::ground; });

  addHotkey (Qt::Key_Minus, MOD_alt, [this] { mainGui->flattenTool->changeRadius(-0.01f); }, [this] { return terrainMode == editing_mode::flatten_blur; });

  addHotkey ( Qt::Key_Minus
            , MOD_alt
            , [&]
              {
                mainGui->texturingTool->change_radius(-0.1f);
              }
            , [this] { return terrainMode == editing_mode::paint; }
            );

  addHotkey (Qt::Key_Minus, MOD_shift, [] { gWorld->fogdistance -= 60.0f; });

  addHotkey (Qt::Key_1, MOD_shift, [this] { movespd = 15.0f; });
  addHotkey (Qt::Key_2, MOD_shift, [this] { movespd = 50.0f; });
  addHotkey (Qt::Key_3, MOD_shift, [this] { movespd = 200.0f; });
  addHotkey (Qt::Key_4, MOD_shift, [this] { movespd = 800.0f; });
  addHotkey (Qt::Key_1, MOD_alt, [this] { mainGui->texturingTool->set_brush_level(0.0f); });
  addHotkey (Qt::Key_2, MOD_alt, [this] { mainGui->texturingTool->set_brush_level(255.0f* 0.25f); });
  addHotkey (Qt::Key_3, MOD_alt, [this] { mainGui->texturingTool->set_brush_level(255.0f* 0.5f); });
  addHotkey (Qt::Key_4, MOD_alt, [this] { mainGui->texturingTool->set_brush_level(255.0f* 0.75f); });
  addHotkey (Qt::Key_5, MOD_alt, [this] { mainGui->texturingTool->set_brush_level(255.0f); });

  addHotkey (Qt::Key_1, MOD_none, [this] { set_editing_mode (editing_mode::ground); });
  addHotkey (Qt::Key_2, MOD_none, [this] { set_editing_mode (editing_mode::flatten_blur); });
  addHotkey (Qt::Key_3, MOD_none, [this] { set_editing_mode (editing_mode::paint); });
  addHotkey (Qt::Key_4, MOD_none, [this] { set_editing_mode (editing_mode::holes); });
  addHotkey (Qt::Key_5, MOD_none, [this] { set_editing_mode (editing_mode::areaid); });
  addHotkey (Qt::Key_6, MOD_none, [this] { set_editing_mode (editing_mode::flags); });
  addHotkey (Qt::Key_7, MOD_none, [this] { set_editing_mode (editing_mode::water); });
  addHotkey (Qt::Key_8, MOD_none, [this] { set_editing_mode (editing_mode::light); });
  addHotkey (Qt::Key_9, MOD_none, [this] { set_editing_mode (editing_mode::mccv); });
  addHotkey (Qt::Key_0, MOD_none, [this] { set_editing_mode (editing_mode::object); });

  addHotkey (Qt::Key_0, MOD_ctrl, [] { boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->doodadset = 0; }, [] { return gWorld->IsSelection(eEntry_WMO); });
  addHotkey (Qt::Key_1, MOD_ctrl, [] { boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->doodadset = 1; }, [] { return gWorld->IsSelection(eEntry_WMO); });
  addHotkey (Qt::Key_2, MOD_ctrl, [] { boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->doodadset = 2; }, [] { return gWorld->IsSelection(eEntry_WMO); });
  addHotkey (Qt::Key_3, MOD_ctrl, [] { boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->doodadset = 3; }, [] { return gWorld->IsSelection(eEntry_WMO); });
  addHotkey (Qt::Key_4, MOD_ctrl, [] { boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->doodadset = 4; }, [] { return gWorld->IsSelection(eEntry_WMO); });
  addHotkey (Qt::Key_5, MOD_ctrl, [] { boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->doodadset = 5; }, [] { return gWorld->IsSelection(eEntry_WMO); });
  addHotkey (Qt::Key_6, MOD_ctrl, [] { boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->doodadset = 6; }, [] { return gWorld->IsSelection(eEntry_WMO); });
  addHotkey (Qt::Key_7, MOD_ctrl, [] { boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->doodadset = 7; }, [] { return gWorld->IsSelection(eEntry_WMO); });
  addHotkey (Qt::Key_8, MOD_ctrl, [] { boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->doodadset = 8; }, [] { return gWorld->IsSelection(eEntry_WMO); });
  addHotkey (Qt::Key_9, MOD_ctrl, [] { boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->doodadset = 9; }, [] { return gWorld->IsSelection(eEntry_WMO); });

  // CAPS warning
  mainGui->capsWarning = new ui::caps_warning;
  mainGui->capsWarning->hide();

  // Water unable to save warning
  mainGui->waterSaveWarning = new ui::water_save_warning;
  mainGui->waterSaveWarning->hide();

  // modelimport
  mainGui->objectEditor->modelImport = new UIModelImport(this);

  // helper models
  mainGui->HelperModels = new UIHelperModels(this);
}

MapView::MapView( float _camera_ah0
                , float _camera_av0
                , math::vector_3d camera_pos
                , math::vector_3d camera_lookat
                )
  : _camera_ah(_camera_ah0)
  , _camera_av(_camera_av0)
  , _camera_lookat (camera_lookat)
  , _GUIDisplayingEnabled(true)
  , mTimespeed(0.0f)
{
  setWindowTitle ("Noggit Studio - " STRPRODUCTVER);

  LastClicked = nullptr;

  // load cursor settings
  if (boost::filesystem::exists("noggit.conf"))
  {
    ConfigFile myConfigfile = ConfigFile("noggit.conf");
    if (myConfigfile.keyExists("RedColor") && myConfigfile.keyExists("GreenColor") && myConfigfile.keyExists("BlueColor") && myConfigfile.keyExists("AlphaColor"))
    {
      cursor_color.x = myConfigfile.read<float>("RedColor");
      cursor_color.y = myConfigfile.read<float>("GreenColor");
      cursor_color.z = myConfigfile.read<float>("BlueColor");
      cursor_color.w = myConfigfile.read<float>("AlphaColor");
    }

    if (myConfigfile.keyExists("CursorType"))
    {
      cursor_type = myConfigfile.read<int>("CursorType");
    }
  }

  setFocusPolicy (Qt::StrongFocus);
  setMouseTracking (true);

  moving = strafing = updown = lookat = turn = 0.0f;

  mousedir = -1.0f;

  movespd = SPEED;

  lastBrushUpdate = 0;

  look = false;
  mViewMode = eViewMode_3D;

  gWorld->camera = camera_pos;

  connect (this, &QObject::destroyed
          , []
            {
              TextureManager::report();
              ModelManager::report();
              WMOManager::report();

              app.asyncLoader->stop();
              app.asyncLoader->join();

              MPQArchive::unloadAllMPQs();
              gListfile.clear();

              LogDebug << "Exited" << std::endl;

              QApplication::quit();
            }
          );

  _startup_time.start();
  _update_every_event_loop.start (0);
  connect (&_update_every_event_loop, &QTimer::timeout, [this] { update(); });
}

  void MapView::initializeGL()
  {
    opengl::context::scoped_setter const _ (::gl, context());
    gl.clearColor (0.0f, 0.0f, 0.0f, 1.0f);
    video.init (width(), height());

    app.initFont();
    createGUI();

  set_editing_mode (editing_mode::ground);


  // Set camera y (height) position to current ground height plus some space.
  math::vector_3d t = math::vector_3d(0, 0, 0);
  tile_index tile(gWorld->camera);
  if (!gWorld->mapIndex.tileLoaded(tile))
  {
    gWorld->mapIndex.loadTile(tile);
  }

  gWorld->GetVertex(gWorld->camera.x, gWorld->camera.z, &t);

  // min elevation according to https://wowdev.wiki/AreaTable.dbc
  //! \ todo use the current area's MinElevation
  if (t.y < -5000.0f)
  {
    //! \todo use the height of a model/wmo of the tile (or the map) ?
    t.y = 0.0f;
  }

  gWorld->camera.y = t.y + 50.0f;

    gl.enableClientState (GL_VERTEX_ARRAY);
    gl.enableClientState (GL_NORMAL_ARRAY);
    gl.enableClientState (GL_TEXTURE_COORD_ARRAY);
  }

  void MapView::paintGL()
  {
    opengl::context::scoped_setter const _ (::gl, context());

    makeCurrent();
    const qreal now(_startup_time.elapsed() / 1000.0);
    tick (now - _last_update);
    _last_update = now;

    gl.clear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    display();
  }

  void MapView::resizeGL (int width, int height)
  {
    opengl::context::scoped_setter const _ (::gl, context());
    video.resize (width, height);
    mainGui->resize();
  }

// void MapView::mainLoop()
// {
//     const Uint8 appState(SDL_GetAppState());
//     const bool isActiveApplication((appState & SDL_APPACTIVE) != 0);
//     const bool hasInputFocus((appState & SDL_APPINPUTFOCUS) != 0);
//     SDL_Event event;
//     while (SDL_PollEvent(&event))
//     {
//       else if (hasInputFocus)
//       {
//         if (event.type == SDL_KEYDOWN)
//         {
//           keyPressEvent (&event.key);
//         }
//         else if (event.type == SDL_KEYUP)
//         {
//           keyReleaseEvent (&event.key);
//         }
//       }
//     }
// }

MapView::~MapView()
{
  delete mainGui;
  mainGui = nullptr;
  delete gWorld;
  gWorld = nullptr;
}

void MapView::tick (float dt)
{
#ifdef _WIN32
  if (tabletActive)
  {
    PACKET pkt;
    while (gpWTPacketsGet(hCtx, 1, &pkt) > 0) //this is a while because we really only want the last packet.
    {
      pressure = pkt.pkNormalPressure;
    }
  }
#endif

  // start unloading tiles
  gWorld->mapIndex.enterTile (tile_index (gWorld->camera));
  gWorld->mapIndex.unloadTiles (tile_index (gWorld->camera));

  dt = std::min(dt, 1.0f);

  update_cursor_pos();

#ifdef _WIN32
  if (app.tabletActive && Settings::getInstance()->tabletMode)
  {
    switch (terrainMode)
    {
    case editing_mode::ground:
      mainGui->terrainTool->setRadius((float)app.pressure / 20.0f);
    case editing_mode::flatten_blur:
      mainGui->flattenTool->setRadius((float)app.pressure / 20.0f);
      break;
    case editing_mode::paint:
      mainGui->texturingTool->change_pressure((float)app.pressure / 2048.0f);
      break;
    case editing_mode::mccv:
      mainGui->shaderTool->setTabletControlValue((float)app.pressure);
      break;
    }
  }
#endif

  if (hasFocus())
  {
    math::vector_3d dir(1.0f, 0.0f, 0.0f);
    math::vector_3d dirUp(1.0f, 0.0f, 0.0f);
    math::vector_3d dirRight(0.0f, 0.0f, 1.0f);
    math::rotate(0.0f, 0.0f, &dir.x, &dir.y, math::degrees(_camera_av));
    math::rotate(0.0f, 0.0f, &dir.x, &dir.z, math::degrees(_camera_ah));

    if (_mod_shift_down)
    {
      dirUp.x = 0.0f;
      dirUp.y = 1.0f;
      dirRight *= 0.0f; //! \todo  WAT?
    }
    else if (_mod_ctrl_down)
    {
      dirUp.x = 0.0f;
      dirUp.y = 1.0f;
      math::rotate(0.0f, 0.0f, &dirUp.x, &dirUp.y, math::degrees(_camera_av));
      math::rotate(0.0f, 0.0f, &dirRight.x, &dirRight.y, math::degrees(_camera_av));
      math::rotate(0.0f, 0.0f, &dirUp.x, &dirUp.z, math::degrees(_camera_ah));
      math::rotate(0.0f, 0.0f, &dirRight.x, &dirRight.z, math::degrees(_camera_ah));
    }
    else
    {
      math::rotate(0.0f, 0.0f, &dirUp.x, &dirUp.z, math::degrees(_camera_ah));
      math::rotate(0.0f, 0.0f, &dirRight.x, &dirRight.z, math::degrees(_camera_ah));
    }
    auto Selection = gWorld->GetCurrentSelection();
    if (Selection)
    {
      // update rotation editor if the selection has changed
      if (!lastSelected || lastSelected != Selection)
      {
        mainGui->rotationEditor->select(*Selection);
      }

      bool canMoveObj = !mainGui->rotationEditor->hasFocus();

      // Set move scale and rotate for numpad keys
      if (_mod_ctrl_down && _mod_shift_down)  moveratio = 0.1f;
      else if (_mod_shift_down) moveratio = 0.01f;
      else if (_mod_ctrl_down) moveratio = 0.005f;
      else moveratio = 0.001f;

      if (canMoveObj && (keyx != 0 || keyy != 0 || keyz != 0 || keyr != 0 || keys != 0))
      {
        // Move scale and rotate with numpad keys
        if (Selection->which() == eEntry_WMO)
        {
          gWorld->updateTilesWMO(boost::get<selected_wmo_type> (*Selection));
          boost::get<selected_wmo_type> (*Selection)->pos.x += keyx * moveratio;
          boost::get<selected_wmo_type> (*Selection)->pos.y += keyy * moveratio;
          boost::get<selected_wmo_type> (*Selection)->pos.z += keyz * moveratio;
          boost::get<selected_wmo_type> (*Selection)->dir.y += keyr * moveratio * 5;

          boost::get<selected_wmo_type> (*Selection)->recalcExtents();
          gWorld->updateTilesWMO(boost::get<selected_wmo_type> (*Selection));
          mainGui->rotationEditor->updateValues();
        }

        if (Selection->which() == eEntry_Model)
        {
          gWorld->updateTilesModel(boost::get<selected_model_type> (*Selection));
          boost::get<selected_model_type> (*Selection)->pos.x += keyx * moveratio;
          boost::get<selected_model_type> (*Selection)->pos.y += keyy * moveratio;
          boost::get<selected_model_type> (*Selection)->pos.z += keyz * moveratio;
          boost::get<selected_model_type> (*Selection)->dir.y += keyr * moveratio * 5;
          boost::get<selected_model_type> (*Selection)->sc += keys * moveratio / 50;
          boost::get<selected_model_type> (*Selection)->recalcExtents();
          gWorld->updateTilesModel(boost::get<selected_model_type> (*Selection));
          mainGui->rotationEditor->updateValues();
        }
      }

      math::vector_3d ObjPos;
      if (gWorld->IsSelection(eEntry_Model))
      {
        //! \todo  Tell me what this is.
        ObjPos = boost::get<selected_model_type> (*Selection)->pos - gWorld->camera;
        math::rotate(0.0f, 0.0f, &ObjPos.x, &ObjPos.y, math::degrees(_camera_av));
        math::rotate(0.0f, 0.0f, &ObjPos.x, &ObjPos.z, math::degrees(_camera_ah));
        ObjPos.x = std::abs(ObjPos.x);
      }

      // moving and scaling objects
      //! \todo  Alternatively automatically align it to the terrain.
      if (MoveObj && canMoveObj)
      {
        ObjPos.x = 80.0f;
        if (Selection->which() == eEntry_WMO)
        {
          gWorld->updateTilesWMO(boost::get<selected_wmo_type> (*Selection));

          if (_mod_shift_down)
          {
            boost::get<selected_wmo_type> (*Selection)->pos += mv * dirUp * ObjPos.x;
            boost::get<selected_wmo_type> (*Selection)->pos -= mh * dirRight * ObjPos.x;
          }
          else
          {
            if (Environment::getInstance()->moveModelToCursorPos)
            {
              boost::get<selected_wmo_type> (*Selection)->pos.x = _cursor_pos.x - objMoveOffset.x;
              boost::get<selected_wmo_type> (*Selection)->pos.z = _cursor_pos.z - objMoveOffset.z;
            }
            else
            {
              boost::get<selected_wmo_type> (*Selection)->pos += mv * dirUp * ObjPos.x;
              boost::get<selected_wmo_type> (*Selection)->pos -= mh * dirRight * ObjPos.x;
            }
          }

          boost::get<selected_wmo_type> (*Selection)->recalcExtents();
          gWorld->updateTilesWMO(boost::get<selected_wmo_type> (*Selection));
          mainGui->rotationEditor->updateValues();
        }
        else if (Selection->which() == eEntry_Model)
        {
          gWorld->updateTilesModel(boost::get<selected_model_type> (*Selection));
          if (_mod_alt_down)
          {
            float ScaleAmount = pow(2.0f, mv * 4.0f);

            boost::get<selected_model_type> (*Selection)->sc *= ScaleAmount;
            if (boost::get<selected_model_type> (*Selection)->sc > 63.9f)
              boost::get<selected_model_type> (*Selection)->sc = 63.9f;
            else if (boost::get<selected_model_type> (*Selection)->sc < 0.00098f)
              boost::get<selected_model_type> (*Selection)->sc = 0.00098f;
          }
          else
          {
            if (_mod_shift_down)
            {
              boost::get<selected_model_type> (*Selection)->pos += mv * dirUp * ObjPos.x;
              boost::get<selected_model_type> (*Selection)->pos -= mh * dirRight * ObjPos.x;
            }
            else
            {
              if (Environment::getInstance()->moveModelToCursorPos)
              {
                boost::get<selected_model_type> (*Selection)->pos.x = _cursor_pos.x - objMoveOffset.x;
                boost::get<selected_model_type> (*Selection)->pos.z = _cursor_pos.z - objMoveOffset.z;
              }
              else
              {
                boost::get<selected_model_type> (*Selection)->pos += mv * dirUp * ObjPos.x;
                boost::get<selected_model_type> (*Selection)->pos -= mh * dirRight * ObjPos.x;
              }

            }
          }

          mainGui->rotationEditor->updateValues();
          boost::get<selected_model_type> (*Selection)->recalcExtents();
          gWorld->updateTilesModel(boost::get<selected_model_type> (*Selection));
        }
      }


      // rotating objects
      if (look && canMoveObj)
      {
        float * lTarget = nullptr;
        bool lModify = false;

        if (Selection->which() == eEntry_Model)
        {
          lModify = _mod_shift_down | _mod_ctrl_down | _mod_alt_down;
          if (_mod_shift_down)
            lTarget = &boost::get<selected_model_type> (*Selection)->dir.y;
          else if (_mod_ctrl_down)
            lTarget = &boost::get<selected_model_type> (*Selection)->dir.x;
          else if (_mod_alt_down)
            lTarget = &boost::get<selected_model_type> (*Selection)->dir.z;

        }
        else if (Selection->which() == eEntry_WMO)
        {
          lModify = _mod_shift_down | _mod_ctrl_down | _mod_alt_down;
          if (_mod_shift_down)
            lTarget = &boost::get<selected_wmo_type> (*Selection)->dir.y;
          else if (_mod_ctrl_down)
            lTarget = &boost::get<selected_wmo_type> (*Selection)->dir.x;
          else if (_mod_alt_down)
            lTarget = &boost::get<selected_wmo_type> (*Selection)->dir.z;

        }

        if (lModify && lTarget)
        {
          gWorld->updateTilesEntry(*Selection);

          *lTarget = *lTarget + rh + rv;

          if (*lTarget > 360.0f)
            *lTarget = *lTarget - 360.0f;
          else if (*lTarget < -360.0f)
            *lTarget = *lTarget + 360.0f;

          mainGui->rotationEditor->updateValues();

          if (Selection->which() == eEntry_WMO)
          {
            boost::get<selected_wmo_type> (*Selection)->recalcExtents();
            gWorld->updateTilesWMO(boost::get<selected_wmo_type> (*Selection));
          }
          else if (Selection->which() == eEntry_Model)
          {
            boost::get<selected_model_type> (*Selection)->recalcExtents();
            gWorld->updateTilesModel(boost::get<selected_model_type> (*Selection));
          }
        }
      }

      mh = 0;
      mv = 0;
      rh = 0;
      rv = 0;


      if (leftMouse && Selection->which() == eEntry_MapChunk)
      {
        bool underMap = gWorld->isUnderMap(_cursor_pos);

        switch (terrainMode)
        {
        case editing_mode::ground:
          if (mViewMode == eViewMode_3D && !underMap)
          {
            if (_mod_shift_down)
            {
              mainGui->terrainTool->changeTerrain(_cursor_pos, 7.5f * dt);
            }
            else if (_mod_ctrl_down)
            {
              mainGui->terrainTool->changeTerrain(_cursor_pos, -7.5f * dt);
            }
          }
          break;
        case editing_mode::flatten_blur:
          if (mViewMode == eViewMode_3D && !underMap)
          {
            if (_mod_shift_down)
            {
              mainGui->flattenTool->flatten(_cursor_pos, dt);
            }
            else if (_mod_ctrl_down)
            {
              mainGui->flattenTool->blur(_cursor_pos, dt);
            }
          }
          break;
        case editing_mode::paint:
          if (_mod_shift_down && _mod_ctrl_down && _mod_alt_down)
          {
            // clear chunk texture
            if (mViewMode == eViewMode_3D && !underMap)
              gWorld->eraseTextures(_cursor_pos);
            else if (mViewMode == eViewMode_2D)
              gWorld->eraseTextures({CHUNKSIZE * 4.0f * video.ratio() * (static_cast<float>(_last_mouse_pos.x()) / static_cast<float>(video.xres()) - 0.5f) / _2d_zoom + gWorld->camera.x, 0.f, CHUNKSIZE * 4.0f * (static_cast<float>(_last_mouse_pos.y()) / static_cast<float>(video.yres()) - 0.5f) / _2d_zoom + gWorld->camera.z});
          }
          else if (_mod_ctrl_down)
          {
            // Pick texture
            mainGui->TexturePicker->getTextures(*gWorld->GetCurrentSelection());
          }
          else  if (_mod_shift_down && !!UITexturingGUI::getSelectedTexture())
          {
            if (mViewMode == eViewMode_3D && !underMap)
            {
              mainGui->texturingTool->paint(_cursor_pos, dt, *UITexturingGUI::getSelectedTexture());
            }
            else if (mViewMode == eViewMode_2D)
            {
              math::vector_3d pos( CHUNKSIZE * 4.0f * video.ratio() * ((float)_last_mouse_pos.x() / (float)video.xres() - 0.5f ) / _2d_zoom
                                  , 0.0f
                                  , CHUNKSIZE * 4.0f * ((float)_last_mouse_pos.y() / (float)video.yres() - 0.5f) / _2d_zoom
                                  );

              pos += gWorld->camera;
              mainGui->texturingTool->paint(pos, dt, *UITexturingGUI::getSelectedTexture());
            }
          }
          break;

        case editing_mode::holes:
          if (mViewMode == eViewMode_3D)
          {
            // no undermap check here, else it's impossible to remove holes
            if (_mod_shift_down)
            {
              auto pos (boost::get<selected_chunk_type> (*Selection).position);
              gWorld->setHole(pos, _mod_alt_down, false);
            }
            else if (_mod_ctrl_down && !underMap)
            {
              gWorld->setHole(_cursor_pos, _mod_alt_down, true);
            }
          }
          break;
        case editing_mode::areaid:
          if (mViewMode == eViewMode_3D && !underMap)
          {
            if (_mod_shift_down)
            {
              // draw the selected AreaId on current selected chunk
              gWorld->setAreaID(_cursor_pos, _selected_area_id, false);
            }
            else if (_mod_ctrl_down)
            {
              // pick areaID from chunk
              MapChunk* chnk (boost::get<selected_chunk_type> (*gWorld->GetCurrentSelection()).chunk);
              int newID = chnk->getAreaID();
              _selected_area_id = newID;
              mainGui->ZoneIDBrowser->setZoneID(newID);
            }
          }
          break;
        case editing_mode::flags:
          if (mViewMode == eViewMode_3D && !underMap)
          {
            if (_mod_shift_down)
            {
              gWorld->mapIndex.setFlag(true, _cursor_pos, FLAG_IMPASS);
            }
            else if (_mod_ctrl_down)
            {
              gWorld->mapIndex.setFlag(false, _cursor_pos, FLAG_IMPASS);
            }
          }
          break;
        case editing_mode::water:
          if (mViewMode == eViewMode_3D && !underMap)
          {
            if (_mod_shift_down)
            {
              mainGui->guiWater->paintLiquid(_cursor_pos, true);
            }
            else if (_mod_ctrl_down)
            {
              mainGui->guiWater->paintLiquid(_cursor_pos, false);
            }
          }
          break;
        case editing_mode::mccv:
          if (mViewMode == eViewMode_3D && !underMap)
          {
            if (_mod_shift_down)
            {
              mainGui->shaderTool->changeShader(_cursor_pos, dt, true);
            }
            if (_mod_ctrl_down)
            {
              mainGui->shaderTool->changeShader(_cursor_pos, dt, false);
            }
          }
          break;
        }
      }
    }

    if (mViewMode != eViewMode_2D)
    {
      if (turn != 0.0f)
      {
        _camera_ah += turn;
        mainGui->minimapWindow->changePlayerLookAt(math::degrees (_camera_ah));
      }
      if (lookat)
      {
        _camera_av += lookat;
        if (_camera_av < -80.0f)
          _camera_av = -80.0f;
        else if (_camera_av > 80.0f)
          _camera_av = 80.0f;
        mainGui->minimapWindow->changePlayerLookAt(math::degrees (_camera_ah));
      }
      if (moving)
        gWorld->camera += dir * dt * movespd * moving;
      if (strafing)
      {
        math::vector_3d right = dir % math::vector_3d(0.0f, 1.0f, 0.0f);
        right.normalize();
        gWorld->camera += right * dt * movespd * strafing;
      }
      if (updown)
        gWorld->camera += math::vector_3d(0.0f, dt * movespd * updown, 0.0f);

      _camera_lookat = gWorld->camera + dir;
    }
    else
    {
      if (moving)
        gWorld->camera.z -= dt * movespd * moving / (_2d_zoom * 1.5f);
      if (strafing)
        gWorld->camera.x += dt * movespd * strafing / (_2d_zoom * 1.5f);
      if (updown)
        _2d_zoom *= pow(2.0f, dt * updown * 4.0f);

      _2d_zoom = std::min(std::max(_2d_zoom, 0.1f), 2.0f);
    }
  }
  else
  {
    leftMouse = false;
    rightMouse = false;
    look = false;
    MoveObj = false;

    moving = 0;
    strafing = 0;
    updown = 0;
  }

  mainGui->texturingTool->update_brushes();


  gWorld->time += this->mTimespeed * dt;


  gWorld->animtime += dt * 1000.0f;
  globalTime = static_cast<int>(gWorld->animtime);

  gWorld->tick (dt);

  lastSelected = gWorld->GetCurrentSelection();

  if (!MapChunkWindow->hidden() && gWorld->GetCurrentSelection() && gWorld->GetCurrentSelection()->which() == eEntry_MapChunk)
  {
    UITexturingGUI::setChunkWindow(boost::get<selected_chunk_type> (*gWorld->GetCurrentSelection()).chunk);
  }
}

selection_result MapView::intersect_result(bool terrain_only)
{
  // during rendering we multiply perspective * view
  // so we need the same order here and then invert.
  math::vector_3d const pos
    ( ( ( math::perspective ( video.fov()
                            , video.ratio()
                            , video.nearclip()
                            , video.farclip()
                            )
        * math::look_at ( gWorld->camera
                        , _camera_lookat
                        , { 0.0f, 1.0f, 0.0f }
                        )
        ).inverted()
      * video.normalized_device_coords (_last_mouse_pos.x(), _last_mouse_pos.y())
      ).xyz_normalized_by_w()
    );

  math::ray ray (gWorld->camera, pos - gWorld->camera);

  selection_result results
    ( gWorld->intersect ( ray
                        , terrain_only
                        , terrainMode == editing_mode::object
                        , _draw_terrain
                        , _draw_wmo
                        , _draw_models
                        , _draw_hidden_models ? std::unordered_set<WMO*>() : _hidden_map_objects
                        , _draw_hidden_models ? std::unordered_set<Model*>() : _hidden_models
                        )
    );

  std::sort ( results.begin()
            , results.end()
            , [](selection_entry const& lhs, selection_entry const& rhs)
              {
                return lhs.first < rhs.first;
              }
            );

  return results;
}

void MapView::doSelection (bool selectTerrainOnly)
{
  selection_result results(intersect_result(selectTerrainOnly));

  if (results.empty())
  {
    gWorld->SetCurrentSelection (boost::none);
  }
  else
  {
    auto const& hit (results.front().second);
    gWorld->SetCurrentSelection (hit);

    _cursor_pos = hit.which() == eEntry_Model ? boost::get<selected_model_type>(hit)->pos
      : hit.which() == eEntry_WMO ? boost::get<selected_wmo_type>(hit)->pos
      : hit.which() == eEntry_MapChunk ? boost::get<selected_chunk_type>(hit).position
      : throw std::logic_error("bad variant");
  }
}

void MapView::update_cursor_pos()
{
  selection_result results (intersect_result (true));

  if (!results.empty())
  {
    auto const& hit(results.front().second);
    // hit cannot be something else than a chunk
    _cursor_pos = boost::get<selected_chunk_type>(hit).position;
  }
}

void MapView::displayGUIIfEnabled()
{
  if (_GUIDisplayingEnabled)
  {
    video.set2D();

    gl.enable(GL_BLEND);
    gl.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    opengl::texture::disable_texture(1);
    opengl::texture::enable_texture(0);

    gl.disable(GL_DEPTH_TEST);
    gl.disable(GL_CULL_FACE);
    gl.disable(GL_LIGHTING);
    gl.color4f(1.0f, 1.0f, 1.0f, 1.0f);

    opengl::texture::disable_texture(0);

    mainGui->setTilemode(mViewMode != eViewMode_3D);
    mainGui->render();

    opengl::texture::enable_texture(0);
  }
}

void MapView::displayViewMode_2D()
{
  video.setTileMode();
  gWorld->drawTileMode ( _camera_ah
                       , gWorld->camera
                       , _draw_lines
                       , _2d_zoom
                       );


  const float mX = (CHUNKSIZE * 4.0f * video.ratio() * (static_cast<float>(_last_mouse_pos.x()) / static_cast<float>(video.xres()) - 0.5f) / _2d_zoom + gWorld->camera.x) / CHUNKSIZE;
  const float mY = (CHUNKSIZE * 4.0f * (static_cast<float>(_last_mouse_pos.y()) / static_cast<float>(video.yres()) - 0.5f) / _2d_zoom + gWorld->camera.z) / CHUNKSIZE;

  // draw brush
  {
    opengl::scoped::matrix_pusher const matrix;

    gl.scalef(_2d_zoom, _2d_zoom, 1.0f);
    gl.translatef(-gWorld->camera.x / CHUNKSIZE, -gWorld->camera.z / CHUNKSIZE, 0);

    gl.color4f(1.0f, 1.0f, 1.0f, 0.5f);
    opengl::texture::set_active_texture(1);
    opengl::texture::disable_texture();
    opengl::texture::set_active_texture(0);
    opengl::texture::enable_texture();

    mainGui->texturingTool->bind_brush_texture();

    const float tRadius = mainGui->texturingTool->brush_radius() / CHUNKSIZE;// *_2d_zoom;
    gl.begin(GL_QUADS);
    gl.texCoord2f(0.0f, 0.0f);
    gl.vertex3f(mX - tRadius, mY + tRadius, 0);
    gl.texCoord2f(1.0f, 0.0f);
    gl.vertex3f(mX + tRadius, mY + tRadius, 0);
    gl.texCoord2f(1.0f, 1.0f);
    gl.vertex3f(mX + tRadius, mY - tRadius, 0);
    gl.texCoord2f(0.0f, 1.0f);
    gl.vertex3f(mX - tRadius, mY - tRadius, 0);
    gl.end();
  }

  displayGUIIfEnabled();
}

void MapView::displayViewMode_3D()
{
  //! \note Select terrain below mouse, if no item selected or the item is map.
  if (!gWorld->IsSelection(eEntry_Model) && !gWorld->IsSelection(eEntry_WMO) && Settings::getInstance()->AutoSelectingMode)
  {
    doSelection(true);
  }

  video.set3D();

  //! \ todo: make the current tool return the radius
  float radius = 0.0f, hardness = 0.0f, inner_radius = 0.0f, angle = 0.0f, orientation = 0.0f;
  math::vector_3d ref_pos;
  bool angled_mode = false, use_ref_pos = false;

  switch (terrainMode)
  {
  case editing_mode::ground:
    radius = mainGui->terrainTool->brushRadius();
    inner_radius = mainGui->terrainTool->innerRadius();
    break;
  case editing_mode::flatten_blur:
    radius = mainGui->flattenTool->brushRadius();
    angle = mainGui->flattenTool->angle();
    orientation = mainGui->flattenTool->orientation();
    ref_pos = mainGui->flattenTool->ref_pos();
    angled_mode = mainGui->flattenTool->angled_mode();
    use_ref_pos = mainGui->flattenTool->use_ref_pos();
    break;
  case editing_mode::paint:
    radius = mainGui->texturingTool->brush_radius();
    hardness = mainGui->texturingTool->hardness();
    break;
  case editing_mode::water:
    radius = mainGui->guiWater->brushRadius();
    angle = mainGui->guiWater->angle();
    orientation = mainGui->guiWater->orientation();
    ref_pos = mainGui->guiWater->ref_pos();
    angled_mode = mainGui->guiWater->angled_mode();
    use_ref_pos = mainGui->guiWater->use_ref_pos();
    break;
  case editing_mode::mccv:
    radius = mainGui->shaderTool->brushRadius();
    break;
  }

  gWorld->draw ( _cursor_pos
               , cursor_color
               , cursor_type
               , radius
               , hardness
               , mainGui->texturingTool->highlight_paintable_chunks()
               , _draw_contour
               , inner_radius
               , ref_pos
               , angle
               , orientation
               , use_ref_pos
               , angled_mode
               , terrainMode == editing_mode::paint
               , terrainMode == editing_mode::flags
               , terrainMode == editing_mode::water
               , terrainMode == editing_mode::areaid
               , terrainMode
               , gWorld->camera
               , _camera_lookat
               , _draw_mfbo
               , _draw_wireframe
               , _draw_lines
               , _draw_terrain
               , _draw_wmo
               , _draw_water
               , _draw_wmo_doodads
               , _draw_models
               , _draw_model_animations
               , _draw_hole_lines || terrainMode == editing_mode::holes
               , _draw_models_with_box
               , _draw_hidden_models ? std::unordered_set<WMO*>() : _hidden_map_objects
               , _draw_hidden_models ? std::unordered_set<Model*>() : _hidden_models
               , _area_id_colors
               );

  displayGUIIfEnabled();
}

void MapView::display()
{
  //! \todo  Get this out or do it somehow else. This is ugly and is a senseless if each draw.
  if (Saving)
  {
    video.setTileMode();
    gWorld->saveMap();
    Saving = false;
  }

  switch (mViewMode)
  {
  case eViewMode_2D:
    displayViewMode_2D();
    break;

  case eViewMode_3D:
    displayViewMode_3D();
    break;
  }
}

void MapView::keyPressEvent (QKeyEvent *event)
{
  if (event->key() == Qt::Key_CapsLock)
    mainGui->capsWarning->show();

  if (handleHotkeys (event))
    return;

  if (event->key() == Qt::Key_Shift)
    _mod_shift_down = true;

  if (event->key() == Qt::Key_Alt)
    _mod_alt_down = true;

  if (event->key() == Qt::Key_Control)
    _mod_ctrl_down = true;

  if (event->key() == Qt::Key_Space)
    _mod_space_down = true;

  // movement
  if (event->key() == Qt::Key_W)
  {
    key_w = true;
    moving = 1.0f;
  }

  if (event->key() == Qt::Key_Up)
  {
    lookat = 0.75f;
  }

  if (event->key() == Qt::Key_Down)
  {
    lookat = -0.75f;
  }

  if (event->key() == Qt::Key_Left)
  {
    turn = -0.75f;
  }

  if (event->key() == Qt::Key_Right)
  {
    turn = 0.75f;
  }

  // save
  if (event->key() == Qt::Key_S)
    moving = -1.0f;

  if (event->key() == Qt::Key_A)
    strafing = -1.0f;

  if (event->key() == Qt::Key_D)
    strafing = 1.0f;

  if (event->key() == Qt::Key_E)
    updown = -1.0f;

  if (event->key() == Qt::Key_Q)
    updown = 1.0f;

  // position correction with num pad
  //! \todo revive
  /*
  if (event->key() == SDLK_KP8)
    keyx = -1;

  if (event->key() == SDLK_KP2)
    keyx = 1;

  if (event->key() == SDLK_KP6)
    keyz = -1;

  if (event->key() == SDLK_KP4)
    keyz = 1;

  if (event->key() == SDLK_KP1)
    keyy = -1;

  if (event->key() == SDLK_KP3)
    keyy = 1;

  if (event->key() == SDLK_KP7)
    keyr = 1;

  if (event->key() == SDLK_KP9)
    keyr = -1;
  */

  // fog distance or brush radius
  if (event->key() == Qt::Key_Plus)
  {
    //change selected model size
    if (gWorld->HasSelection() && gWorld->GetCurrentSelection()->which() != eEntry_MapChunk)
      keys = 1;
  }

  if (event->key() == Qt::Key_Minus)
  {
    //change selected model size
    if (gWorld->HasSelection() && gWorld->GetCurrentSelection()->which() != eEntry_MapChunk)
      keys = -1;
  }
}

void MapView::keyReleaseEvent (QKeyEvent* event)
{
  if (event->key() == Qt::Key_Shift)
    _mod_shift_down = false;

  if (event->key() == Qt::Key_Alt)
    _mod_alt_down = false;

  if (event->key() == Qt::Key_Control)
    _mod_ctrl_down = false;

  if (event->key() == Qt::Key_Space)
    _mod_space_down = false;

  // movement
  if (event->key() == Qt::Key_W)
  {
    key_w = false;
    if (!(leftMouse && rightMouse) && moving > 0.0f)
    {
      moving = 0.0f;
    }
  }

  if (event->key() == Qt::Key_S && moving < 0.0f)
  {
    moving = 0.0f;
  }

  if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
    lookat = 0.0f;

  if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right)
    turn = 0.0f;

  if (event->key() == Qt::Key_D && strafing > 0.0f)
  {
    strafing = 0.0f;
  }

  if (event->key() == Qt::Key_A && strafing < 0.0f)
  {
    strafing = 0.0f;
  }

  if (event->key() == Qt::Key_Q && updown > 0.0f)
    updown = 0.0f;

  if (event->key() == Qt::Key_E && updown < 0.0f)
    updown = 0.0f;

  //! \todo revive
  /*
  if (event->key() == SDLK_KP8)
    keyx = 0;

  if (event->key() == SDLK_KP2)
    keyx = 0;

  if (event->key() == SDLK_KP6)
    keyz = 0;

  if (event->key() == SDLK_KP4)
    keyz = 0;

  if (event->key() == SDLK_KP1)
    keyy = 0;

  if (event->key() == SDLK_KP3)
    keyy = 0;

  if (event->key() == SDLK_KP7)
    keyr = 0;

  if (event->key() == SDLK_KP9)
    keyr = 0;
  */

  if (event->key() == Qt::Key_Minus || event->key() == Qt::Key_Plus)
    keys = 0;
}

void MapView::inserObjectFromExtern(int model)
{
  if (!gWorld->HasSelection())
    return;

  std::string m2_to_add;

  const char* filesToAdd[15] = { ""
                                 , ""
                                 , "World\\Scale\\humanmalescale.m2"
                                 , "World\\Scale\\50x50.m2"
                                 , "World\\Scale\\100x100.m2"
                                 , "World\\Scale\\250x250.m2"
                                 , "World\\Scale\\500x500.m2"
                                 , "World\\Scale\\1000x1000.m2"
                                 , "World\\Scale\\50yardradiusdisc.m2"
                                 , "World\\Scale\\200yardradiusdisc.m2"
                                 , "World\\Scale\\777yardradiusdisc.m2"
                                 , "World\\Scale\\50yardradiussphere.m2"
                                 , "World\\Scale\\200yardradiussphere.m2"
                                 , "World\\Scale\\777yardradiussphere.m2"
                                 , ""
  };

  m2_to_add = filesToAdd[model];

  math::vector_3d selectionPosition;
  switch (gWorld->GetCurrentSelection()->which())
  {
  case eEntry_Model:
    selectionPosition = boost::get<selected_model_type> (*gWorld->GetCurrentSelection())->pos;
    break;
  case eEntry_WMO:
    selectionPosition = boost::get<selected_wmo_type> (*gWorld->GetCurrentSelection())->pos;
    break;
  case eEntry_MapChunk:
    selectionPosition = boost::get<selected_chunk_type> (*gWorld->GetCurrentSelection()).position;
    break;
  }

  if (!MPQFile::exists(m2_to_add))
  {
    LogError << "Failed adding " << m2_to_add << ". It was not in any MPQ." << std::endl;
  }

  gWorld->addM2(m2_to_add, selectionPosition, false);
  //! \todo Memoryleak: These models will never get deleted.
}

void MapView::mouseMoveEvent (QMouseEvent* event)
{
  QLineF const relative_movement (_last_mouse_pos, event->pos());

  if (look && !(_mod_shift_down || _mod_ctrl_down || _mod_alt_down || _mod_space_down))
  {
    _camera_ah += relative_movement.dx() / XSENS;
    _camera_av += mousedir * relative_movement.dy() / YSENS;
    if (_camera_av < -80.0f)
      _camera_av = -80.0f;
    else if (_camera_av > 80.0f)
      _camera_av = 80.0f;

    mainGui->minimapWindow->changePlayerLookAt(math::degrees (_camera_ah));
  }

  if (MoveObj)
  {
    mh = -video.ratio()*relative_movement.dx() / static_cast<float>(video.xres());
    mv = -relative_movement.dy() / static_cast<float>(video.yres());
  }
  else
  {
    mh = 0.0f;
    mv = 0.0f;
  }

  if (_mod_shift_down || _mod_ctrl_down || _mod_alt_down || _mod_space_down)
  {
    rh = relative_movement.dx() / XSENS * 5.0f;
    rv = relative_movement.dy() / YSENS * 5.0f;
  }

  if (rightMouse && _mod_alt_down)
  {
    if (terrainMode == editing_mode::ground)
    {
      mainGui->terrainTool->changeInnerRadius(relative_movement.dx() / 100.0f);
    }
    if (terrainMode == editing_mode::paint)
    {
      mainGui->texturingTool->change_hardness(relative_movement.dx() / 300.0f);
    }
  }

  if (rightMouse && _mod_shift_down)
  {
    if (terrainMode == editing_mode::ground)
    {
      mainGui->terrainTool->moveVertices(-relative_movement.dy() / YSENS);
    }
  }


  if (rightMouse && _mod_space_down)
  {
    mainGui->terrainTool->setOrientRelativeTo(_cursor_pos);
  }

  if (leftMouse && _mod_alt_down)
  {
    switch (terrainMode)
    {
    case editing_mode::ground:
      mainGui->terrainTool->changeRadius(relative_movement.dx() / XSENS);
      break;
    case editing_mode::flatten_blur:
      mainGui->flattenTool->changeRadius(relative_movement.dx() / XSENS);
      break;
    case editing_mode::paint:
      mainGui->texturingTool->change_radius(relative_movement.dx() / XSENS);
      break;
    case editing_mode::water:
      mainGui->guiWater->changeRadius(relative_movement.dx() / XSENS);
      break;
    case editing_mode::mccv:
      mainGui->shaderTool->changeRadius(relative_movement.dx() / XSENS);
      break;
    }
  }

  if (leftMouse && _mod_space_down)
  {
    switch (terrainMode)
    {
    case editing_mode::ground:
      mainGui->terrainTool->changeSpeed(relative_movement.dx() / 30.0f);
      break;
    case editing_mode::flatten_blur:
      mainGui->flattenTool->changeSpeed(relative_movement.dx() / 30.0f);
      break;
    case editing_mode::paint:
      mainGui->texturingTool->change_pressure(relative_movement.dx() / 300.0f);
      break;
    case editing_mode::mccv:
      mainGui->shaderTool->changeSpeed(relative_movement.dx() / XSENS);
      break;
    }
  }

  if (leftMouse && LastClicked)
  {
    LastClicked->processLeftDrag((float)(event->pos().x() - 4), (float)(event->pos().y() - 4), relative_movement.dx(), relative_movement.dy());
  }

  if (mViewMode == eViewMode_2D && leftMouse && _mod_alt_down && _mod_shift_down)
  {
    strafing = ((relative_movement.dx() / XSENS) / -1) * 5.0f;
    moving = (relative_movement.dy() / YSENS) * 5.0f;
  }

  if (mViewMode == eViewMode_2D && rightMouse && _mod_shift_down)
  {
    updown = (relative_movement.dy() / YSENS);
  }

  mainGui->mouse_moved (event->pos().x(), event->pos().y());

  _last_mouse_pos = event->pos();

  checkWaterSave(); // ????? \todo Move to somewhere more appropriate.
}

void MapView::selectModel(selection_type entry)
{
  mainGui->objectEditor->copy(entry);
}

void MapView::mousePressEvent (QMouseEvent* event)
{
  switch (event->button())
  {
  case Qt::LeftButton:
    leftMouse = true;
    break;

  case Qt::RightButton:
    rightMouse = true;
    break;

  case Qt::MiddleButton:
    if (gWorld->HasSelection())
    {
      MoveObj = true;
      auto selection = gWorld->GetCurrentSelection();
      math::vector_3d objPos;
      if (selection->which() == eEntry_WMO)
      {
        objPos = boost::get<selected_wmo_type> (*selection)->pos;
      }
      else if (selection->which() == eEntry_Model)
      {
        objPos = boost::get<selected_model_type> (*selection)->pos;
      }

      objMoveOffset = _cursor_pos - objPos;
    }

    break;
  }

  if (leftMouse && rightMouse)
  {
    // Both buttons
    moving = 1.0f;
  }
  else if (leftMouse)
  {
    LastClicked = mainGui->processLeftClick (event->pos().x(), event->pos().y());
    if (mViewMode == eViewMode_3D && !LastClicked)
    {
      doSelection(false);
    }
  }
  else if (rightMouse)
  {
    look = true;
  }
}

void MapView::wheelEvent (QWheelEvent* event)
{
  //! \todo don't just use distance but delta
  float delta (event->angleDelta().y());

  if (delta > 0.f)
  {
    if (terrainMode == editing_mode::ground)
    {
      if (_mod_alt_down)
      {
        mainGui->terrainTool->changeAngle(_mod_ctrl_down ? 0.2f : 2.0f);
      }
      else if (_mod_shift_down)
      {
        mainGui->terrainTool->changeOrientation(_mod_ctrl_down ? 1.0f : 10.0f);
      }
    }
    else if (terrainMode == editing_mode::flatten_blur)
    {
      if (_mod_alt_down)
      {
        mainGui->flattenTool->changeOrientation(_mod_ctrl_down ? 1.0f : 10.0f);
      }
      else if (_mod_shift_down)
      {
        mainGui->flattenTool->changeAngle(_mod_ctrl_down ? 0.2f : 2.0f);
      }
      else if (_mod_space_down)
      {
        mainGui->flattenTool->changeHeight(1.0f);
      }
    }
    else if (terrainMode == editing_mode::paint)
    {
      if (_mod_space_down)
      {
        mainGui->texturingTool->change_brush_level(10.0f);
      }
      else if (_mod_alt_down)
      {
        mainGui->texturingTool->change_spray_size(1.0f);
      }
      else if (_mod_shift_down)
      {
        mainGui->texturingTool->change_spray_pressure(0.25);
      }
    }
    else if (terrainMode == editing_mode::water)
    {
      if (_mod_alt_down)
      {
        mainGui->guiWater->changeOrientation(_mod_ctrl_down ? 1.0f : 10.0f);
      }
      else if (_mod_shift_down)
      {
        mainGui->guiWater->changeAngle(_mod_ctrl_down ? 0.2f : 2.0f);
      }
      else if (_mod_space_down)
      {
        mainGui->guiWater->change_height(_mod_ctrl_down ? 0.1f : 1.0f);
      }
    }
  }
  else
  {
    if (terrainMode == editing_mode::ground)
    {
      if (_mod_alt_down)
      {
        mainGui->terrainTool->changeAngle(_mod_ctrl_down ? -0.2f : -2.0f);
      }
      else if (_mod_shift_down)
      {
        mainGui->terrainTool->changeOrientation(_mod_ctrl_down ? -1.0f : -10.0f);
      }
    }
    else if (terrainMode == editing_mode::flatten_blur)
    {
      if (_mod_alt_down)
      {
        mainGui->flattenTool->changeOrientation(_mod_ctrl_down ? -1.0f : -10.0f);
      }
      else if (_mod_shift_down)
      {
        mainGui->flattenTool->changeAngle(_mod_ctrl_down ? -0.2f : -2.0f);
      }
      else if (_mod_space_down)
      {
        mainGui->flattenTool->changeHeight(-1.0f);
      }
    }
    else if (terrainMode == editing_mode::paint)
    {
      if (_mod_space_down)
      {
        mainGui->texturingTool->change_brush_level(-10.0f);
      }
      else if (_mod_alt_down)
      {
        mainGui->texturingTool->change_spray_size(-1.0f);
      }
      else if (_mod_shift_down)
      {
        mainGui->texturingTool->change_spray_pressure(-0.25);
      }
    }
    else if (terrainMode == editing_mode::water)
    {
      if (_mod_alt_down)
      {
        mainGui->guiWater->changeOrientation(_mod_ctrl_down ? -1.0f : -10.0f);
      }
      else if (_mod_shift_down)
      {
        mainGui->guiWater->changeAngle(_mod_ctrl_down ? -0.2f : -2.0f);
      }
      else if (_mod_space_down)
      {
        mainGui->guiWater->change_height(_mod_ctrl_down ? -0.1f : -1.0f);
      }
    }
  }
}

void MapView::mouseReleaseEvent (QMouseEvent* event)
{
  switch (event->button())
  {
  case Qt::LeftButton:
    leftMouse = false;

    if (LastClicked)
      LastClicked->processUnclick();

    if (!key_w && moving > 0.0f)
      moving = 0.0f;

    if (mViewMode == eViewMode_2D)
    {
      strafing = 0;
      moving = 0;
    }
    break;

  case Qt::RightButton:
    rightMouse = false;

    look = false;

    if (!key_w && moving > 0.0f)
      moving = 0.0f;

    if (mViewMode == eViewMode_2D)
      updown = 0;

    break;

  case Qt::MiddleButton:
    MoveObj = false;
    break;
  }
}

void MapView::checkWaterSave()
{
  tile_index const current (gWorld->camera);

  if (!gWorld->mapIndex.hasTile (current) || gWorld->canWaterSave(current))
  {
    mainGui->waterSaveWarning->hide();
  }
  else
  {
    mainGui->waterSaveWarning->show();
  }
}

void MapView::closeEvent (QCloseEvent* event)
{
  event->ignore();
  prompt_exit();
}


void MapView::prompt_exit()
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle("Exit map editor and return to menu?");
    msgBox.setText("Exit the map editor and return to menu?");
    msgBox.setInformativeText("Any unsaved changes will be lost.");
    msgBox.addButton("Return to Menu", QMessageBox::AcceptRole);
    QPushButton *continueButton = msgBox.addButton("Continue Editing", QMessageBox::RejectRole);
    msgBox.setDefaultButton(continueButton);
    msgBox.setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    msgBox.exec();

    if (msgBox.buttonRole(msgBox.clickedButton()) == QMessageBox::AcceptRole)
    {
      deleteLater();
    }
}

void MapView::prompt_save_current() const
{
  if ( QMessageBox::warning
         ( nullptr
         , "Save (only) current map tile"
         , "This can cause a collision bug when placing objects between two ADT borders!\n\n"
           "If you often use this function, we recommend you to use the 'Save all'"
           "function as often as possible to get the collisions right."
         , QMessageBox::Save | QMessageBox::Cancel
         , QMessageBox::Cancel
         ) == QMessageBox::Save
     )
  {
    gWorld->mapIndex.savecurrent();
  }
}

void MapView::addHotkey(Qt::Key key, size_t modifiers, std::function<void()> function, std::function<bool()> condition)
{
  hotkeys.emplace_front (key, modifiers, function, condition);
}

bool MapView::handleHotkeys(QKeyEvent* event)
{
  size_t modifier = (event->modifiers() == Qt::NoModifier) ? (MOD_none) : (
    ((event->modifiers() & Qt::ShiftModifier) ? MOD_shift : 0) |
    ((event->modifiers() & Qt::ControlModifier) ? MOD_ctrl : 0) |
    ((event->modifiers() & Qt::AltModifier) ? MOD_alt : 0) |
    ((event->modifiers() & Qt::MetaModifier) ? MOD_meta : 0));

  for (auto&& hotkey : hotkeys)
  {
    if (event->key() == hotkey.key && modifier == hotkey.modifiers && hotkey.condition())
    {
      hotkey.function();
      return true;
    }
  }
  return false;
}
