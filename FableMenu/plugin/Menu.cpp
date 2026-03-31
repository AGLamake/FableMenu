#include "Menu.h"
#include "Settings.h"

#include "../helper/eKeyboardMan.h"
#include "../helper/eMouse.h"

#include "../Fable.h"
#include "../utils/MemoryMgr.h"

#include "../gui/notifications.h"
#include "../gui/imgui/imgui.h"
#include "../gui/gui_impl_dx9.h"

#include "FreeCamera.h"

#include <iostream>
#include <Windows.h>

#include "../utils/CsvReader.h"
#include "../utils/IniReader.h"
#include <string>
#include <map>
#include "../gui/log.h"
#include "../fable/Thing.h"



using namespace Memory::VP;
static FableMenu* g_pMenu = nullptr;

FableMenu& GetMenu() {
	if (!g_pMenu) {
		g_pMenu = new FableMenu();
	}
	return *g_pMenu;
}

static CIniReader* g_pLang = nullptr;

CIniReader& GetLang() {
	if (!g_pLang) {
		g_pLang = new CIniReader("fablemenu_lang.ini");
	}
	return *g_pLang;
}

bool FableMenu::ms_bFreeCam = false;
bool FableMenu::m_bCustomCameraPos = false;
bool FableMenu::ms_bDisableHUD = false;
bool FableMenu::m_bCustomCameraFOV = false;
bool FableMenu::ms_bChangeTime = false;
bool FableMenu::ms_bDisableCreateParticle = false;
float FableMenu::m_fTime = 0.0f;
std::vector<CThing*> FableMenu::m_vCreatedParticles;
std::vector<CThing*> FableMenu::m_vAttachedParticles;
bool FableMenu::ms_bSlowmotion = false;

static void ShowHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static void ShowWarnMarker(const char* desc)
{
    ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "(!)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

struct Translation {
    std::string original;
    std::string translate;
    std::string note;
    std::string crash;
    std::string limited;
};

struct DictionaryInfo {
    std::string filename;
    std::map<std::string, Translation>& dictionary;
    int columns;
};

std::map<std::string, Translation> object_dir;
std::map<std::string, Translation> crt_dir;
std::map<std::string, Translation> attack_dir;
std::map<std::string, Translation> brain_dir;
std::map<std::string, Translation> creature_modes_dir;
std::map<std::string, Translation> player_modes_dir;
std::map<std::string, Translation> expressions_dir;
std::map<std::string, Translation> creature_animations_dir;
std::map<std::string, Translation> factions_dir;
std::map<std::string, Translation> building_dir;
std::map<std::string, Translation> holysites_dir;

template<typename K, typename V>
std::pair<K, V> findValue(const std::map<K, V>& map, const std::string& value) {
    for (const auto& pair : map) {
        if (pair.second.original == value || pair.first == value) {
            return pair;
        }
    }
    return std::pair<K, V>();
}

template<typename K, typename V>
std::pair<K, V> getByKey(const std::map<K, V>& map, const K& key) {
    auto it = map.find(key);
    if (it != map.end()) {
        return *it;
    }
    return std::pair<K, V>();
}

std::pair<std::string, Translation> getById(const std::map<std::string, Translation>& map, int id) {
    std::string key = std::to_string(id);
    auto it = map.find(key);
    if (it != map.end()) {
        return *it;
    }
    return std::pair<std::string, Translation>();
}

template<typename K, typename V>
bool isPairEmpty(const std::pair<K, V>& pair) {
    return pair.first.empty();
}

bool LoadDictionary(DictionaryInfo& dictInfo) {
    try {
        CCsvReader reader(dictInfo.filename.c_str(), true);

        if (!reader.OpenCsv()) {
            throw std::string("Не удалось открыть файл: ") + dictInfo.filename;
        }

        std::vector<std::string> columns;
        while (!(columns = reader.ReadLine()).empty()) {
            if (columns.size() >= 2) {
                Translation trans;

                switch (dictInfo.columns) {
                case 3:
                    trans.original = columns[1];
                    trans.translate = columns[2];
                    trans.note = "";
                    trans.crash = "";
                    trans.limited = "0";
                    break;

                case 4:
                    trans.original = columns[1];
                    trans.translate = columns[2];
                    trans.note = columns[3];
                    trans.crash = "";
                    trans.limited = "0";
                    break;

                case 5:
                    trans.original = columns[1];
                    trans.translate = columns[2];
                    trans.note = columns[3];
                    trans.crash = columns[4];
                    trans.limited = "0";
                    break;

                case 6:
                    trans.original = columns[1];
                    trans.translate = columns[2];
                    trans.note = columns[3];
                    trans.crash = columns[4];
                    trans.limited = columns[5];
                    break;

                default:
                    trans.original = columns[1];
                    trans.translate = columns[2];
                    trans.note = "";
                    trans.crash = "";
                    trans.limited = "0";
                    break;
                }

                dictInfo.dictionary[columns[0]] = trans;
            }
        }

        return true;

    }
    catch (const std::string& error) {
        Notifications->SetNotificationTime(5000);
        Notifications->PushNotification(error.c_str());
    }
    catch (const char* error) {
        Notifications->SetNotificationTime(5000);
        Notifications->PushNotification(error);
    }
    catch (const std::exception& ex) {
        Notifications->SetNotificationTime(5000);
        Notifications->PushNotification(ex.what());
    }

    return false;
}

void LoadAllDictionaries() {
    std::vector<DictionaryInfo> dictionaries = {
        {"creature_animations.csv", creature_animations_dir, 3},
        {"factions.csv", factions_dir, 3},
        {"expressions.csv", expressions_dir, 3},
        {"player_modes.csv", player_modes_dir, 3},
        {"creature_modes.csv", creature_modes_dir, 3},
        {"brain.csv", brain_dir, 3},
        {"attack.csv", attack_dir, 3},
        {"building.csv", building_dir, 3},
        {"holysites.csv", holysites_dir, 3},

        {"obj.csv", object_dir, 4},

        {"crt.csv", crt_dir, 5}
    };

    for (auto& dict : dictionaries) {
        LoadDictionary(dict);
    }
}


FableMenu::FableMenu()
{
    LoadAllDictionaries();

    sprintf(szFactionName, szFactions[0]);
}

void FableMenu::OnActivate()
{
    m_bIsActive ^= 1;
}

void FableMenu::Draw()
{

    if (!m_bIsActive)
        return;

    ImGui::GetIO().MouseDrawCursor = true;

	ImGui::Begin(GetLang().ReadString("Window", "main_window", "FableMenu by ermaccer"), &m_bIsActive, ImGuiWindowFlags_MenuBar);
    {
        ImGui::SetWindowSize({ 600, 500 }, ImGuiCond_Once);
        if (ImGui::BeginMenuBar())
        {
			if (ImGui::BeginMenu(GetLang().ReadString("Menu", "menu_settings", "Settings")))
            {
                m_bSubmenuActive[SM_Settings] = true;
                ImGui::EndMenu();
            }
			if (ImGui::BeginMenu(GetLang().ReadString("Menu", "menu_help", "Help")))
            {
				if (ImGui::MenuItem(GetLang().ReadString("Menu", "menu_help_creatures", "Creature List")))
                {
                    m_bSubmenuActive[SM_Creature_List] = true;
                }
                if (ImGui::MenuItem(GetLang().ReadString("Menu", "menu_help_particles", "Particle List")))
                {
                    m_bSubmenuActive[SM_Particle_List] = true;
                }
				if (ImGui::MenuItem(GetLang().ReadString("Menu", "menu_help_objects", "Objects List")))
                {
                    m_bSubmenuActive[SM_Object_List] = true;
                }
				if (ImGui::BeginMenu(GetLang().ReadString("Menu", "menu_about", "About")))
                {
					ImGui::MenuItem(GetLang().ReadString("Menu", "menu_about_version" FABLEMENU_VERSION, "Version: " FABLEMENU_VERSION));
					ImGui::MenuItem((GetLang().ReadString("Menu", "menu_about_date" __DATE__, "Date: " __DATE__)));
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (CMainGameComponent::Get())
        {
            if (ImGui::BeginTabBar("##tabs"))
            {
				if (ImGui::BeginTabItem(GetLang().ReadString("Tabs", "tab_hero", "Hero")))
                {
                    DrawHeroTab();
                    ImGui::EndTabItem();
                }
				if (ImGui::BeginTabItem(GetLang().ReadString("Tabs", "tab_player", "Player")))
                {
                    DrawPlayerTab();
                    ImGui::EndTabItem();
                }
				if (ImGui::BeginTabItem(GetLang().ReadString("Tabs", "tab_creatures", "Creatures")))
                {
                    DrawCreaturesTab();
                    ImGui::EndTabItem();
                }
				if (ImGui::BeginTabItem(GetLang().ReadString("Tabs", "tab_objects", "Objects")))
                {
                    DrawObjectsTab();
                    ImGui::EndTabItem();
                }
				if (ImGui::BeginTabItem(GetLang().ReadString("Tabs", "tab_camera", "Camera")))
                {
                    DrawCameraTab();
                    ImGui::EndTabItem();
                }
				if (ImGui::BeginTabItem(GetLang().ReadString("Tabs", "tab_world", "World")))
                {
                    DrawWorldTab();
                    ImGui::EndTabItem();
                }
				if (ImGui::BeginTabItem(GetLang().ReadString("Tabs", "tab_quest", "Quest")))
                {
                    DrawQuestTab();
                    ImGui::EndTabItem();
                }
				if (ImGui::BeginTabItem(GetLang().ReadString("Tabs", "tab_misc", "Misc.")))
                {
                    DrawMiscTab();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        else
			ImGui::TextWrapped(GetLang().ReadString("Preload", "not_ready", "Not ready!"));
    }
    ImGui::End();

    if (m_bSubmenuActive[SM_Settings])
        DrawSettings();

    if (m_bSubmenuActive[SM_Particle_List])
        DrawParticleList();

    if (m_bSubmenuActive[SM_Creature_List])
        DrawCreatureList();

    if (m_bSubmenuActive[SM_Object_List])
        DrawObjectList();
}

void FableMenu::Process()
{
    if (m_bCustomCameraFOV)
    {
        Nop(0xA0BEE2, 3);
    }
    else
    {
        //89 41 2C 
        Patch<char>(0xA0BEE2, 0x89);
        Patch<char>(0xA0BEE2 + 1, 0x41);
        Patch<char>(0xA0BEE2 + 2, 0x2C);
    }

    if (ms_bChangeTime)
    {
        Nop(0x6BBA58, 3);
    }
    else
    {
        //D9 56 08 
        Patch<char>(0x6BBA58, 0xD9);
        Patch<char>(0x6BBA58 + 1, 0x56);
        Patch<char>(0x6BBA58 + 2, 0x08);
    }

    if (m_bNoBodyGuardsLimit)
        Patch<char>(0xE60689 + 2, 0xFF);
    else
        Patch<char>(0xE60689 + 2, 2);
    
    if (m_bForceLoadRegion)
    {
        *(int*)NProgressDisplay::PProgressDisplay = 0;
    }
    else if (!*(int*)NProgressDisplay::PProgressDisplay)
    {
        CAProgressDisplay* progressDisplay = (CAProgressDisplay*)GameMalloc(179);
        NProgressDisplay::InitialiseProgressDisplay(progressDisplay);
    }

    // Disable fadeout
    Patch(0x4A411E, { (unsigned char)(m_bForceLoadRegion ? 0x74 : 0x75) });
    // RegionLoadScreenWasFadedOut
    Patch(0x4A415D, { (BYTE)m_bForceLoadRegion });
}

void FableMenu::DrawHeroTab()
{
    CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer();
    if (plr)
    {
        CThing* t = plr->GetCharacterThing();
        CTCHeroStats* stats = (CTCHeroStats*)t->GetTC(TCI_HERO_STATS);
        CTCHeroMorph* morph = (CTCHeroMorph*)t->GetTC(TCI_APPEARANCE_MORPH);
        CTCHeroExperience* exp = (CTCHeroExperience*)t->GetTC(TCI_HERO_EXPERIENCE);
        CTCLook* look = (CTCLook*)t->GetTC(TCI_LOOK);
        CTCHero* hero = (CTCHero*)t->GetTC(TCI_HERO);
        CTCHaste* haste = (CTCHaste*)t->GetTC(TCI_HASTE);
        CTCPhysicsStandard* physics = (CTCPhysicsStandard*)t->GetTC(TCI_PHYSICS);
        CTCCarrying* carrying = (CTCCarrying*)t->GetTC(TCI_CARRYING);
        if (stats)
        {
			if (ImGui::CollapsingHeader(GetLang().ReadString("Data", "data_text", "Data")))
            {
				ImGui::InputFloat(GetLang().ReadString("Data", "data_health", "Health"), &t->m_fHealth);
				ImGui::InputFloat(GetLang().ReadString("Data", "data_max_health", "Max. Health"), &t->m_fMaxHealth);
                ImGui::InputInt(GetLang().ReadString("Data", "data_max_will", "Will"), &stats->m_nStamina, 0);
                ImGui::InputInt(GetLang().ReadString("Data", "data_max_will", "Max. Will"), &stats->m_nMaxStamina, 0);
                ImGui::InputInt(GetLang().ReadString("Data", "data_gold", "Gold"), &stats->m_nMoney);
                ImGui::InputFloat(GetLang().ReadString("Data", "data_age", "Age"), &stats->m_fAge);
                if (hero)
                {
                    ImGui::Checkbox(GetLang().ReadString("Data", "data_can_use_weapons", "Can Use Weapons"), &hero->m_bCanUseWeapons);
                    ImGui::Checkbox(GetLang().ReadString("Data", "data_can_use_will", "Can Use Will"), &hero->m_bCanUseWill);

                    ImGui::Separator();
                    ImGui::Text(GetLang().ReadString("Data", "data_renown_text", "Renown Data"));
                    ImGui::Separator();

                    ImGui::InputInt(GetLang().ReadString("Data", "data_renown", "Renown"), &stats->m_nRenownTotal);
                    if (ImGui::SliderInt(GetLang().ReadString("Data", "data_renown_level", "Renown Level"), &stats->m_nRenownLevel, 1, stats->m_nRenownMaxLevel))
                    {
                        stats->CheckForNewExpressions();
                    }
                    ImGui::InputInt(GetLang().ReadString("Data", "data_renown_points", "Renown Points In Level"), &stats->m_nRenownPointsInLevel);
                    ImGui::InputInt(GetLang().ReadString("Data", "data_morality", "Morality"), &stats->m_nMorality);

                    static int titleObjectID = 1220;
                    const char* defPrefix = "OBJECT_HERO_TITLE";
                    size_t prefixLen = strlen(defPrefix);

                    ImGui::Separator();
                    ImGui::Text(GetLang().ReadString("Data", "data_title", "Title"));
                    ImGui::Separator();
                    ImGui::PushItemWidth(-FLT_MIN);

                    std::string currentTitle = "Unknown";
                    auto currentResult = findValue(object_dir, std::to_string(titleObjectID));
                    std::string translate;
                    if (!isPairEmpty(currentResult)) {
                        currentTitle = currentResult.second.original;
						translate = currentResult.second.translate;
                    }

                    if (ImGui::BeginCombo("##title", translate.c_str()))
                    {
                        for (const auto& pair : object_dir)
                        {
                            const std::string& key = pair.first;
                            const Translation& trans = pair.second;

                            if (trans.original.find(defPrefix) == 0)
                            {
                                int id = std::stoi(key);
                                bool is_selected = (titleObjectID == id);

                                if (ImGui::Selectable(trans.translate.c_str(), is_selected))
                                {
                                    titleObjectID = id;
                                }
                                if (is_selected)
                                {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();

                    if (ImGui::Button(GetLang().ReadString("Data", "data_set_title", "Set Title"), { -FLT_MIN, 0 }))
                    {
                        CGameDefinitionManager* defManager = CGameDefinitionManager::GetDefinitionManager();
                        CCharString titleDef((char*)(currentTitle.c_str()));

                        Notifications->SetNotificationTime(2500);
                        Notifications->PushNotification((currentResult.second.translate + " [" + currentResult.second.original + "]").c_str());

                        int titleDefIndex = defManager->GetDefGlobalIndexFromName(&titleDef);
                        hero->SetTitle(titleDefIndex);
                    }
                }
                if (exp)
                {
                    ImGui::Separator();
					ImGui::Text(GetLang().ReadString("Exp", "exp_text", "Experience"));
                    ImGui::Separator();

                    ImGui::InputInt(GetLang().ReadString("Exp", "exp_general", "General"), &exp->m_nGeneralExperience, 0);
                    ImGui::InputInt(GetLang().ReadString("Exp", "exp_strength", "Strength"), &exp->m_pExperience[EXPERIENCE_STRENGTH], 0);
                    ImGui::InputInt(GetLang().ReadString("Exp", "exp_will", "Will"), &exp->m_pExperience[EXPERIENCE_WILL], 0);
                    ImGui::InputInt(GetLang().ReadString("Exp", "exp_skill", "Skill"), &exp->m_pExperience[EXPERIENCE_SKILL], 0);

                    if (ImGui::Button(GetLang().ReadString("Exp", "exp_learn_all", "Learn All Abilities")))
                    {
                        for (int i = 1; i < MAX_NUMBER_OF_HERO_ABILITIES; i++)
                        {
                            t->LearnAbility((EHeroAbility)i, 1, -1, 1);
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(GetLang().ReadString("Exp", "exp_abilities_to_max", "Abilities To Max Level")))
                    {
                        CTCInventoryAbilities* abilities = (CTCInventoryAbilities*)t->GetTC(TCI_HERO_ABILITIES);

                        if (abilities)
                            abilities->ForceAllAbilitesToMaxLevel();
                    }
                    if (ImGui::Button(GetLang().ReadString("Exp", "exp_learn_all_expressions", "Learn All Expressions"), { -FLT_MIN, 0 }))
                    {
                        for (auto expression : expressionNames)
                        {
                            CCharString expressionName((char*)expression);
                            t->LearnExpression(&expressionName, -1, 1);
                        }
                    }
                }
                if (stats)
                {
                    ImGui::Separator();
                    ImGui::Text(GetLang().ReadString("Stats", "stats_text", "Misc."));
                    ImGui::Separator();

                    ImGui::InputFloat(GetLang().ReadString("Stats", "stats_sound_radius_mult", "Sound Radius Multiplier"), &stats->m_fSoundRadiusMultiplier);
                    ImGui::InputFloat(GetLang().ReadString("Stats", "stats_visibility_mult", "Visibility Multiplier"), &stats->m_fVisibilityMultiplier);
                    ImGui::InputFloat(GetLang().ReadString("Stats", "stats_chicken_throw", "Max Chicken Throw"), &stats->m_fMaxChickenThrow);
                    ImGui::Checkbox(GetLang().ReadString("Stats", "stats_has_scripted_sex", "Had Scripted Sex"), &stats->m_bHadScriptedSex);
                    ImGui::Checkbox(GetLang().ReadString("Stats", "stats_has_scripted_gay_sex", "Had Scripted Gay Sex"), &stats->m_bHadScriptedGaySex);
                }
            }
        }
        if (morph)
        {
			if (ImGui::CollapsingHeader(GetLang().ReadString("Morph", "morph_text", "Morph")))
            {
                ImGui::SliderFloat(GetLang().ReadString("Morph", "morph_strength", "Strength"), &morph->m_fStrength, 0.00f, 1.0f);
                ImGui::SliderFloat(GetLang().ReadString("Morph", "morph_berserk", "Berserk"), &morph->m_fBerserk, 0.00f, 1.0f);
                ImGui::SliderFloat(GetLang().ReadString("Morph", "morph_will", "Will"), &morph->m_fWill, 0.00f, 1.0f);
                ImGui::SliderFloat(GetLang().ReadString("Morph", "morph_skill", "Skill"), &morph->m_fSkill, 0.00f, 1.0f);
                ImGui::SliderFloat(GetLang().ReadString("Morph", "morph_age", "Age"), &morph->m_fAge, 0.00f, 1.0f);
                ImGui::SliderFloat(GetLang().ReadString("Morph", "morph_alignment", "Alignment"), &morph->m_fAlign, 0.00f, 1.0f);
                ImGui::SliderFloat(GetLang().ReadString("Morph", "morph_fatness", "Fatness"), &morph->m_fFat, 0.00f, 1.0f);

                ImGui::Checkbox(GetLang().ReadString("Morph", "morph_kid", "Kid"), &morph->m_bKid);
                if (ImGui::Button(GetLang().ReadString("Morph", "morph_update", "Update"), ImVec2(-FLT_MIN, 0)))
                    morph->m_bUpdate = true;
            }
        }
        if (carrying)
        {
            if (ImGui::CollapsingHeader(GetLang().ReadString("Weapon", "weapon_text", "Weapon")))
            {
                CThing* thingPrimarySlot = carrying->GetThingInPrimarySlot();

                if (thingPrimarySlot && thingPrimarySlot->HasTC(TCI_WEAPON))
                {
                    ImGui::Separator();
                    ImGui::Text(GetLang().ReadString("Weapon", "weapon_augmentation", "Augmentations"));
                    ImGui::Separator();

                    CTCObjectAugmentations* augObject = (CTCObjectAugmentations*)thingPrimarySlot->GetTC(TCI_OBJECT_AUGMENTATIONS);
                    int numberOfSlots = augObject->GetNumberOfSlots();
                    static int selectedAug = 0;

                    std::vector<std::pair<std::string, std::string>> augList;
                    for (const auto& pair : object_dir)
                    {
                        const std::string& key = pair.first;
                        const Translation& trans = pair.second;

                        if (key.find("AUGMENTATION") != std::string::npos ||
                            trans.original.find("AUGMENTATION") != std::string::npos)
                        {
                            augList.push_back({ key, trans.original });
                        }
                    }

                    static bool slotsLimit = false;

                    ImGui::Text(GetLang().ReadString("Weapon", "weapon_dmg_multi", "Damage Multiplier: %f"), augObject->GetDamageMultiplier());
                    ImGui::Text(GetLang().ReadString("Weapon", "weapon_exp_multi", "Expirience Multiplier: %f"), augObject->GetExperienceMultiplier());

                    ImGui::BeginChild(GetLang().ReadString("Weapon", "weapon_aug_slots", "Augmention Slots"), { 0, -ImGui::GetFrameHeightWithSpacing() + 200 }, true);
                    if (numberOfSlots == 0)
                    {
                        ImGui::LabelText("", GetLang().ReadString("Weapon", "weapon_no_slots", "No Slots Available"));
                    }
                    else
                    {
                        for (int i = 0; i < numberOfSlots; i++)
                        {
                            CWideString name;
                            augObject->GetAugmentationNameInSlot(&name, i);

                            wchar_t* wstr = name.GetWideStringData();
                            int len = wcslen(wstr);
                            std::string augName(len, '\0');
                            WideCharToMultiByte(CP_UTF8, 0, wstr, len, &augName[0], len, NULL, NULL);

                            std::string displayName = augName;

                            auto result = findValue(object_dir, augName);
                            if (!isPairEmpty(result))
                            {
                                displayName = result.second.translate;
                            }

                            ImGui::LabelText("", "%s", displayName.c_str());

                            ImGui::SameLine();
                            ImGui::PushID(i);
                            if (ImGui::Button(GetLang().ReadString("Weapon", "weapon_btn_set", "Set")))
                            {
                                if (!augList.empty() && selectedAug < augList.size())
                                {
                                    CCharString augDefName((char*)augList[selectedAug].second.c_str());
                                    int augIndex = CGameDefinitionManager::GetDefinitionManager()->GetDefGlobalIndexFromName(&augDefName);
                                    augObject->AttachAugmentationToSlot(augIndex, i);
                                }
                            }
                            ImGui::SameLine();
                            if (ImGui::Button(GetLang().ReadString("Weapon", "weapon_btn_clear", "Clear")))
                            {
                                augObject->RemoveAugmentationFromSlot(i);
                            }
                            ImGui::PopID();
                        }
                    }
                    ImGui::EndChild();

                    ImGui::LabelText("", GetLang().ReadString("Weapon", "weapon_aug_name", "Augmentation Name"));
                    ImGui::PushItemWidth(-FLT_MIN);

                    if (ImGui::BeginCombo("##augmentation", !augList.empty() ?
                        object_dir[augList[selectedAug].first].translate.c_str() : "No augmentations"))
                    {
                        for (size_t n = 0; n < augList.size(); n++)
                        {
                            auto it = object_dir.find(augList[n].first);
                            if (it == object_dir.end()) continue;

                            bool is_selected = (selectedAug == (int)n);
                            if (ImGui::Selectable(it->second.translate.c_str(), is_selected))
                            {
                                selectedAug = (int)n;
                            }
                            if (is_selected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::PopItemWidth();

                    if (ImGui::Checkbox(GetLang().ReadString("Weapon", "weapon_disable_slot_limit", "Disable Slots Limit"), &slotsLimit))
                    {
                        if (slotsLimit)
                        {
                            Nop(0x766D88, 2);
                        }
                        else
                        {
                            Patch(0x766D88, { 0x7D, 0x38 });
                        }
                    }

                    if (ImGui::Button(GetLang().ReadString("Weapon", "weapon_add_new_slot", "Add New Slot"), { -FLT_MIN, 0 }))
                    {
                        augObject->AddNewSlot();
                    }
                }
                else
                {
                    ImGui::Text(GetLang().ReadString("Weapon", "weapon_so_selected_weapon", "No selected weapon"));
                }
            }
        }
        if (haste)
        {
            if (ImGui::CollapsingHeader(GetLang().ReadString("Haste", "haste_text", "Haste")))
            {
                static float combatSpeed = 1;
                ImGui::InputFloat(GetLang().ReadString("Haste", "haste_adr_multi", "Adrenaline Multiplier"), &combatSpeed);
                if (ImGui::Button(GetLang().ReadString("Haste", "haste_set_adr", "Set Adrenaline"), { -FLT_MIN, 0 }))
                {
                    haste->UnsetActionSpeedMultiplier(t);
                    haste->SetActionSpeedMultiplier(t, combatSpeed);
                }
                ImGui::Separator();
                ImGui::Text(GetLang().ReadString("Haste", "haste_movement", "Movement"));
                ImGui::Separator();

                static const char* movementNames[] = {
					GetLang().ReadString("Movement", "mov_slow", "Slow Walk"),
					GetLang().ReadString("Movement", "mov_walk", "Walk"),
					GetLang().ReadString("Movement", "mov_jog", "Jog"),
					GetLang().ReadString("Movement", "mov_run", "Run"),
					GetLang().ReadString("Movement", "mov_roll", "Roll")
                };

                for (int i = 0; i < TOTAL_MOVEMENT_TYPES - 1; i++)
                {
                    float& speed = *(float*)((int)t + 0x18C + (i * sizeof(int)));

                    ImGui::InputFloat(movementNames[i], &speed);
                }

                static int movementType = -1;

                ImGui::Separator();
                ImGui::Text(GetLang().ReadString("Haste", "haste_move_type", "Movement Type"));
                ImGui::Separator();

                ImGui::RadioButton(GetLang().ReadString("Haste", "haste_move_type_default", "Default"), &movementType, DEFAULT_MOVEMENT);
                ImGui::SameLine();
                ImGui::RadioButton(GetLang().ReadString("Haste", "haste_move_type_walk", "Walk Movement"), &movementType, WALK_MOVEMENT);
                ImGui::SameLine();
                ImGui::RadioButton(GetLang().ReadString("Haste", "haste_move_type_jog", "Jog Movement"), &movementType, JOG_MOVEMENT);
                ImGui::SameLine();
                ImGui::RadioButton(GetLang().ReadString("Haste", "haste_move_type_run", "Run Movement"), &movementType, RUN_MOVEMENT);

                Memory::VP::Patch(0x6AB514, { 0xB8, (movementType != DEFAULT_MOVEMENT ? (unsigned char)movementType : (unsigned char)JOG_MOVEMENT) });
                Memory::VP::Patch(0x6AB5BC, { 0xB8, (movementType != DEFAULT_MOVEMENT ? (unsigned char)movementType : (unsigned char)RUN_MOVEMENT) });
            }
        }
		if (ImGui::CollapsingHeader(GetLang().ReadString("Spell", "spell_text", "Spell Data")))
        {
            int& curSummonCreature = *(int*)(0x138306C);
			ImGui::TextWrapped(GetLang().ReadString("Spell", "spell_current", "Current Summon Creature ID"));
            ImGui::SameLine();
			ShowHelpMarker(GetLang().ReadString("Spell", "spell_help", "You can get desired creature ID from World->Cretures section."));
            ImGui::PushItemWidth(-FLT_MIN);
            ImGui::InputInt("##creaturesumid", &curSummonCreature);
            ImGui::PopItemWidth();
            ImGui::Separator();

        }
		if (ImGui::CollapsingHeader(GetLang().ReadString("Input", "input_text", "Input")))
        {
			ImGui::TextWrapped(GetLang().ReadString("Input", "input_help", ""));
			if (ImGui::Button(GetLang().ReadString("Input", "input_disable", "Disable"), { -FLT_MIN, 0 }))
            {
                if (plr)
                    plr->DisableInput();
            }
			if (ImGui::Button(GetLang().ReadString("Input", "input_enable", "Enable"), { -FLT_MIN, 0 }))
            {
                if (plr)
                    plr->EnableInput();
            }
        }
		if (ImGui::CollapsingHeader(GetLang().ReadString("Appearance", "ap_text", "Appearance")))
        {
            DrawAppearanceCollapse(t);
        }
        if (physics)
        {
            if (ImGui::CollapsingHeader(GetLang().ReadString("Physics", "phys_text", "Physics")))
            {
                DrawPhysicsCollapse(t);
                bool isGravityEnabled = physics->IsGravityEnabled();
                if (ImGui::Checkbox(GetLang().ReadString("Physics", "phys_enable_gravity", "Enable Gravity"), &isGravityEnabled))
                    physics->EnableGravity(isGravityEnabled);
                ImGui::Checkbox(GetLang().ReadString("Physics", "phys_enable_collision", "Enable Player Collision"), &NGlobalConsole::EnableHeroThingCollision);
            }
        }
    }
}

void FableMenu::DrawPlayerTab()
{
    CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer();

#ifdef _DEBUG
    ImGui::SetWindowFontScale(0.85f);
    ImGui::Text(GetLang().ReadString("Player", "player_number", "Player Number: %d"), plr->m_dNumber);
    ImGui::SameLine();
    ImGui::Text(GetLang().ReadString("Player", "player_current_mode", "Current Mode: %d"), plr->GetCurrentMode());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();
#endif

    if (ImGui::CollapsingHeader(GetLang().ReadString("Player", "player_modes", "Modes")))
    {
        std::list<enum EPlayerMode> playerModes = plr->m_lPlayerModes;
        int removeID = 0;

        ImGui::BeginChild("#mlist", { 0, -ImGui::GetFrameHeightWithSpacing() - 200}, true);
        for (EPlayerMode mode : playerModes)
        {
            auto result = findValue(player_modes_dir, szPlayerModeNames[mode]);
			std::string displayText = result.second.translate; //  + " (" + result.second.original + ")";
            ImGui::LabelText("", displayText.c_str());
           
            if (playerModes.size() > 1)
            { 
                ImGui::SameLine();
                ImGui::PushID(removeID);
                if (ImGui::Button(GetLang().ReadString("Player", "player_button_remove", "Remove")))
                {
                    plr->RemoveMode(mode);
                }
                ImGui::PopID();
                removeID++;
            }
        }
		ImGui::EndChild();
        static int modeID = 0;

        if (ImGui::BeginCombo(GetLang().ReadString("Player", "player_mode_name", "Mode Name"), getById(player_modes_dir, modeID).second.translate.c_str()))
        {
            for (auto& pair : player_modes_dir)
            {
                const std::string key = pair.first;
                int intKey = std::stoi(key);
                const std::string display_text = pair.second.translate; // + " [" + pair.second.original + "]";
                bool is_selected = (modeID == intKey);
                if (ImGui::Selectable(display_text.c_str(), is_selected)) {
                    modeID = intKey;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
                    
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button(GetLang().ReadString("Player", "player_button_add_mode", "Add Mode"), {-FLT_MIN, 0}))
        {
            plr->AddMode((EPlayerMode)modeID, 0);
        }
        ImGui::Separator();

        static bool aggressiveMode;

        if (ImGui::Checkbox(GetLang().ReadString("Player", "player_aggressive_mode", "Aggressive Mode"), &aggressiveMode))
        {
            if (!FGlobals::GUsePassiveAggressiveMode)
            {
                FGlobals::GUsePassiveAggressiveMode = 1;
            }

            plr->SetAgressiveMode(aggressiveMode);
        }
    }
    if (ImGui::CollapsingHeader(GetLang().ReadString("Player", "player_actions", "Actions")))
    {
        DrawActionsCollapse(plr->GetCharacterThing());
    }
    if (ImGui::CollapsingHeader(GetLang().ReadString("Player", "player_character", "Character")))
    {
        static bool manualInput = false;
        static bool uninitPlayerCharacter = true;
        static bool playerCharacterDefinitionError = false;
        static int characterID = 0;
        static char selectedPlayerDefManually[256];

        if (playerCharacterDefinitionError)
        {
            ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), GetLang().ReadString("Player", "player_invalid_character", "Invalid character definition"));
        }

        static const char* playerCharacterDefs[] =
        {
            "CREATURE_HERO",
            "CREATURE_HERO_BALVERINE",
            "CREATURE_HERO_TRAINING",
            "CREATURE_HERO_CHILD"
        };

        if (!manualInput)
        {
            std::string currentDisplay = playerCharacterDefs[characterID];
            auto result = findValue(crt_dir, playerCharacterDefs[characterID]);
            if (!isPairEmpty(result))
            {
                currentDisplay = result.second.translate;
            }

            if (ImGui::BeginCombo(GetLang().ReadString("Player", "player_character_definition", "Character Definition"), currentDisplay.c_str()))
            {
                for (int n = 0; n < IM_ARRAYSIZE(playerCharacterDefs); n++)
                {
                    std::string displayText = playerCharacterDefs[n];
                    auto result2 = findValue(crt_dir, playerCharacterDefs[n]);
                    if (!isPairEmpty(result2))
                    {
                        displayText = result2.second.translate;
                    }

                    bool is_selected = (characterID == n);
                    if (ImGui::Selectable(displayText.c_str(), is_selected))
                        characterID = n;
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            ImGui::InputText(GetLang().ReadString("Player", "player_character_definition", "Character Definition"), selectedPlayerDefManually, sizeof(selectedPlayerDefManually));
        }

        ImGui::Checkbox(GetLang().ReadString("Player", "player_manual_input", "Manual Input##plrmode"), &manualInput);
        ImGui::SameLine(); ShowWarnMarker(GetLang().ReadString("Player", "player_warn_marker", "Defintion has to be compatible with the hero."));
        ImGui::SameLine();
        ImGui::Checkbox(GetLang().ReadString("Player", "player_uninit_character", "Uninit Character"), &uninitPlayerCharacter);
        ImGui::InputFloat3(GetLang().ReadString("Player", "player_respawn_position", "Repawn Position"), &FGlobals::GOverridePlayerStartPos->X);
        if (ImGui::Button(GetLang().ReadString("Player", "player_get_position", "Get Player Position")))
        {
            *FGlobals::GOverridePlayerStartPos = *plr->GetCharacterThing()->GetPosition();
        }
        if (ImGui::Button(GetLang().ReadString("Player", "player_button_respawn", "Respawn Hero"), { -FLT_MIN, 0 }))
        {
            if (!FGlobals::GOverridePlayerStartPosFromConsole)
            {
                FGlobals::GOverridePlayerStartPosFromConsole = 1;
            }

            FGlobals::GUseRubbishMovementMethod = characterID != 1;

            CCharString defName(manualInput ? selectedPlayerDefManually : (char*)playerCharacterDefs[characterID]);

            if (!CGameDefinitionManager::GetDefinitionManager()->GetDefGlobalIndexFromName(&defName))
            {
                playerCharacterDefinitionError = true;
                return;
            }
            else
            {
                playerCharacterDefinitionError = false;
            }

            if (uninitPlayerCharacter)
            {
                plr->UninitCharacter();
            }

            Patch(0x48A0A6, { { (unsigned char)((BYTE)!uninitPlayerCharacter + 0x74) } });

            plr->InitCharacterAs(&defName);
            plr->AddMode(PLAYER_MODE_VIEW_HERO, 0);
            plr->RemoveMode(PLAYER_MODE_VIEW_HERO);
        }
    }
}

void FableMenu::DrawCreaturesTab()
{
    CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer();
    CThing* playerCharacter = plr->GetCharacterThing();
    CThingSearchTools* search = CMainGameComponent::Get()->GetWorld()->GetThingSearchTools();
    std::list<CThing*> regionCreatures = *search->PeekTypeList(1);

    static char creatureName[512] = { };
    static char creatureNote[512] = { };
    static CVector creaturePosition = {};
    static int creatureId;
    static int creatureCount = 1;
    static bool advanced;
    static bool playerFollower;

    if (ImGui::CollapsingHeader(GetLang().ReadString("Creatures", "creature_text", "Creature Spawner")))
    {
        ImGui::TextWrapped(GetLang().ReadString("Creatures", "creature_position", "Spawn Position (X | Y | Z)"));
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::InputFloat3("", &creaturePosition.X);
        ImGui::PopItemWidth();

        if (ImGui::Button(GetLang().ReadString("Creatures", "creature_get_position", "Get Player Position"), { -FLT_MIN, 0 }))
        {
            if (plr)
                creaturePosition = *playerCharacter->GetPosition();
        }

        ImGui::Checkbox(GetLang().ReadString("Creatures", "creature_advanced_settings", "Advanced Settings"), &advanced);
        if (advanced)
        {
            ImGui::Separator();
            ImGui::InputInt(GetLang().ReadString("Creatures", "creature_owner_id", "Owner ID##creatures"), &creatureId);

            ImGui::TextWrapped(GetLang().ReadString("Creatures", "creature_faction", "Faction"));
            ImGui::PushItemWidth(-FLT_MIN);

            static int selectedFactionIndex = 0;
            static std::vector<std::string> factionOriginalList;
            static std::vector<std::string> factionTranslateList;

            if (factionOriginalList.empty())
            {
                for (const auto& pair : factions_dir)
                {
                    factionOriginalList.push_back(pair.first);
                    factionTranslateList.push_back(pair.second.translate);
                }
            }

            std::string currentDisplay;
            for (size_t i = 0; i < factionOriginalList.size(); i++)
            {
                if (factionOriginalList[i] == szFactionName)
                {
                    currentDisplay = factionTranslateList[i];
                    selectedFactionIndex = i;
                    break;
                }
            }
            if (currentDisplay.empty() && !factionOriginalList.empty())
            {
                currentDisplay = factionTranslateList[0];
                strcpy_s(szFactionName, sizeof(szFactionName), factionOriginalList[0].c_str());
                selectedFactionIndex = 0;
            }

            if (ImGui::BeginCombo("##faclist", currentDisplay.c_str()))
            {
                for (size_t n = 0; n < factionOriginalList.size(); n++)
                {
                    bool is_selected = (szFactionName == factionOriginalList[n]);
                    if (ImGui::Selectable(factionTranslateList[n].c_str(), is_selected))
                    {
                        strcpy_s(szFactionName, sizeof(szFactionName), factionOriginalList[n].c_str());
                        selectedFactionIndex = n;
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            ImGui::Checkbox(GetLang().ReadString("Creatures", "creature_as_follower", "Create as player follower"), &playerFollower);
            ImGui::SameLine();
            ShowHelpMarker(GetLang().ReadString("Creating", "cr_as_follower_help", "Creature will follow and defend player, this only works with some creatures (usually those that are simple enough, eg. swords or spirits)"));
            ImGui::Separator();
        }
        ImGui::Text(GetLang().ReadString("Creatures", "creature_name", "Creature Name"));
        ImGui::SameLine();
        ShowHelpMarker(GetLang().ReadString("Creatures", "create_help_list", "Creature list is available in Help menu."));
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::InputText("##creature", creatureName, sizeof(creatureName));

        ImGui::PopItemWidth();

        if (ButtonAutoSize(GetLang().ReadString("Creatures", "create_paste", "Paste")))
        {
            std::string clip = GetClipboardText();
            strcpy_s(creatureName, sizeof(creatureName), clip.c_str());

            CGameDefinitionManager* defManager = CGameDefinitionManager::GetDefinitionManager();
            CCharString ccsCreatureName((char*)creatureName);
            creatureId = defManager->GetDefGlobalIndexFromName(&ccsCreatureName);
        }
        ImGui::SameLine();
        if (ButtonAutoSize(GetLang().ReadString("Creatures", "creature_get_id", "Get ID")))
        {
            CGameDefinitionManager* defManager = CGameDefinitionManager::GetDefinitionManager();
            CCharString ccsCreatureName((char*)creatureName);
            creatureId = defManager->GetDefGlobalIndexFromName(&ccsCreatureName);
        }

        ImGui::InputInt(GetLang().ReadString("Creatures", "creature_id", "Creature ID"), &creatureId);
        if (creatureId <= 0)
            ImGui::TextWrapped(GetLang().ReadString("Creatures", "creature_invalid_id", "Invalid creature ID!"));
        else
        {
            ImGui::InputInt(GetLang().ReadString("Creatures", "creature_count", "Count"), &creatureCount);

            if (getById(factions_dir, creatureId).second.crash == "1") {
                ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), GetLang().ReadString("Creatures", "creature_crash_warning", "This creature is known to cause crashes when spawned!"));
            }

            if (ImGui::Button(GetLang().ReadString("Creatures", "creature_spawn", "Spawn Creature"), { -FLT_MIN, 0 }))
            {
                for (int i = 1; i <= creatureCount; i++) {
                    CThing* creature = CreateCreature(creatureId, &creaturePosition, 0);
                    if (creature && advanced)
                    {
                        if (plr)
                        {
                            if (!creature->HasTC(TCI_ENEMY))
                            {
                                CCharString enemyTC((char*)"CTCEnemy");
                                creature->AddTC(&enemyTC, 0, 0);
                            }
                            CTCEnemy* enemy = (CTCEnemy*)creature->GetTC(TCI_ENEMY);
                            CCharString faction(szFactionName);
                            enemy->SetFaction(&faction);

                            if (playerFollower)
                            {
                                CIntelligentPointer ptr(creature);

                                CTCRegionFollower* rf = (CTCRegionFollower*)plr->GetCharacterThing()->GetTC(TCI_REGION_FOLLOWER);
                                rf->AddFollower(*ptr);

                                CTCFollowed* pf = (CTCFollowed*)plr->GetCharacterThing()->GetTC(TCI_FOLLOWED);
                                enemy->AddAlly(plr->GetCharacterThing());
                                pf->AddFollower(*ptr, 1);

                                CCharString brainName((char*)"BRAIN_FOLLOW_PLAYER");
                                CGameDefinitionManager* defManager = CGameDefinitionManager::GetDefinitionManager();
                                int brainIndex = defManager->GetDefGlobalIndexFromName(&brainName);
                                int result[1];
                                result[0] = 0;
                                defManager->GetOpinionPersonalitDef(brainIndex, result);
                                creature->SetNewBrain(result[0]);
                            }
                            else
                            {
                                if (strcmp(szFactionName, "FACTION_HERO") == 0)
                                    enemy->AddAlly(plr->GetCharacterThing());
                            }
                        }
                    }
                }
            }
        }
    }
    if (ImGui::CollapsingHeader(GetLang().ReadString("Creatures", "creature_region", "Region Creatures")))
    {
        static std::list<CThing*> filteredCreatures;
        static std::vector<char> creatureDataWindowsOpen;
        static bool displayCreatureFilterOptions;
        static int filteredType = -1;

        std::list<CThing*> creatureList;
        filteredCreatures.clear();

        size_t creaturesInLocation = regionCreatures.size();
        ImGui::Text(GetLang().ReadString("Creatures", "creature_in_region", "Creatures In Location: %d"), creaturesInLocation);
        ImGui::Separator();

        if (!displayCreatureFilterOptions || filteredType == -1)
        {
            creatureList = regionCreatures;
        }
        else
        {
            for (CThing* t : regionCreatures)
            {
                switch (filteredType)
                {
                case 0:
                    if (t->GetCreatureType() == NOT_HUMAN)
                        filteredCreatures.push_back(t);
                    break;
                case 1:
                    if (t->GetCreatureType() == HUMAN_CHILD)
                        filteredCreatures.push_back(t);
                    break;
                case 2:
                    if (t->GetCreatureType() == HUMAN_ADULT)
                        filteredCreatures.push_back(t);
                    break;
                case 3:
                    if (t->GetCreatureType() == HUMAN_ELDERLY)
                        filteredCreatures.push_back(t);
                    break;
                case 4:
                    if (t->HasTC(TCI_BANDIT))
                        filteredCreatures.push_back(t);
                    break;
                case 5:
                    if (t->HasTC(TCI_GUARD))
                        filteredCreatures.push_back(t);
                    break;
                case 6:
                    if (t->HasTC(TCI_SHOP_KEEPER))
                        filteredCreatures.push_back(t);
                    break;
                case 7:
                    if (t->HasTC(TCI_HERO))
                        filteredCreatures.push_back(t);
                    break;
                default:
                    break;
                }
            }
            creatureList = filteredCreatures;
        }

        ImGui::Checkbox(GetLang().ReadString("CreatureFilter", "filter", "Creature Filter"), &displayCreatureFilterOptions);
        if (displayCreatureFilterOptions)
        {
            ImGui::RadioButton(GetLang().ReadString("CreatureFilter", "filter_all", "All"), &filteredType, -1);
            ImGui::SameLine();
            ImGui::RadioButton(GetLang().ReadString("CreatureFilter", "filter_not_humans", "Not Humans"), &filteredType, 0);
            ImGui::SameLine();
            ImGui::RadioButton(GetLang().ReadString("CreatureFilter", "filter_children", "Children"), &filteredType, 1);
            ImGui::RadioButton(GetLang().ReadString("CreatureFilter", "filter_adults", "Adults"), &filteredType, 2);
            ImGui::SameLine();
            ImGui::RadioButton(GetLang().ReadString("CreatureFilter", "filter_elderly", "Elderly"), &filteredType, 3);
            ImGui::SameLine();
            ImGui::RadioButton(GetLang().ReadString("CreatureFilter", "filter_bandits", "Bandits"), &filteredType, 4);
            ImGui::RadioButton(GetLang().ReadString("CreatureFilter", "filter_guards", "Guards"), &filteredType, 5);
            ImGui::SameLine();
            ImGui::RadioButton(GetLang().ReadString("CreatureFilter", "filter_traders", "Traders"), &filteredType, 6);
            ImGui::SameLine();
            ImGui::RadioButton(GetLang().ReadString("CreatureFilter", "filter_heroes", "Heroes"), &filteredType, 7);
        }

        size_t creatureNumber = creatureList.size();
        creatureDataWindowsOpen.resize(creatureNumber, false);

        if (ImGui::Button(GetLang().ReadString("CreatureFilter", "filter_kill_all", "Kill All")))
        {
            for (auto creature : creatureList)
            {
                creature->Kill(true);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(GetLang().ReadString("CreatureFilter", "filter_teleport_all", "Teleport All")))
        {
            for (auto creature : creatureList)
            {
                CTCPhysicsStandard* physics = (CTCPhysicsStandard*)creature->GetTC(TCI_PHYSICS);
                physics->SetPosition(playerCharacter->GetPosition());
            }
        }

        if (creatureNumber != 0)
        {
            int i = 0;
            ImGui::BeginChild("##regionCreatureList", { 0, -ImGui::GetFrameHeightWithSpacing() + 400 }, true);
            for (CThing* creature : creatureList)
            {
                CDefString* defName = creature->GetDefName();
                CCharString buffer;
                CDefString::GetString(&buffer, defName->tablePos);
                char* charDefName = buffer.GetStringData();

                const char* thingName;

                auto trnslate = findValue(crt_dir, charDefName);
                if (!isPairEmpty(trnslate)) {
					thingName = trnslate.second.translate.c_str();
                }
                else {
                    thingName = (const char*)charDefName;;
                }

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.15f, 0.75f, 0.35f, 1.0f));
                ImGui::PushItemWidth(420.f);
                ImGui::LabelText("", thingName);
                ImGui::PopStyleColor();
                ImGui::PopItemWidth();
                ImGui::SameLine();
                char* isOpen = &creatureDataWindowsOpen[i];
                ImGui::PushID(i);
                if (ImGui::Button(GetLang().ReadString("CreatureFilter", "filter_open_data", "Open Data")))
                {
                    *isOpen = !(*isOpen);
                }
                ImGui::PopID();

                char windowTitle[256];
                snprintf(windowTitle, sizeof(windowTitle), "%s##%d", thingName, i);

                if (*isOpen)
                {
                    DrawCreatureData(windowTitle, creature, (bool*)isOpen);
                }
                i++;
            }
            ImGui::EndChild();
        }
        else
        {
            ImGui::Separator();
            ImGui::TextColored({ 1.f, 0.3f, 0.3f, 1.f }, GetLang().ReadString("CreatureFilter", "filter_no_creatures", "No creatures to display"));
        }
    }
    if (ImGui::CollapsingHeader(GetLang().ReadString("Village", "village_text", "Village Data")))
    {
        CTCVillage* village = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer()->GetPNearestTCVillage();

        static char villagerDef[256];
        static bool disableVillager = true;
        static bool toggleGuardVillagers = true;

        if (!village)
        {
            ImGui::TextColored({ 1.f, 0.3f, 0.3f, 1.f }, GetLang().ReadString("Village", "village_no_village", "No Village"));
            return;
        }

        ImGui::InputText(GetLang().ReadString("Village", "village_villager_def", "Villager Definition"), villagerDef, sizeof(villagerDef));

        if (ImGui::Button((const char*)(disableVillager ? GetLang().ReadString("Village", "village_disable_villager", "Disable Villager") : GetLang().ReadString("Village", "village_enable_villager", "Enable Villager"))))
        {
            CCharString defName(villagerDef);
            village->EnableVillagerDefTypes(villagerDef, &defName);

            disableVillager = !disableVillager;
        }
        ImGui::SameLine(); ShowHelpMarker(GetLang().ReadString("Village", "village_help", "Disables creature by definition, works only for village member."));
        ImGui::Separator();
        if (ImGui::Checkbox(GetLang().ReadString("Village", "village_enable_guards", "Enable Guards"), &toggleGuardVillagers))
        {
            village->EnableGuards(toggleGuardVillagers);
        }
        if (ImGui::Button(GetLang().ReadString("Village", "village_clear_crimes", "Clear Crimes"), { -FLT_MIN, 0 }))
        {
            village->ClearCrimes();
        }
        if (ImGui::Button(GetLang().ReadString("Village", "village_limbo", "Village Limbo"), { -FLT_MIN, 0 }))
        {
            village->SetVillageLimbo(1);
        }
    }
}

void FableMenu::DrawObjectsTab()
{
    static CVector position = {};
    static int objectId = 0;
    static char objectName[512] = { };
    CPlayer* player = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer();

    if (ImGui::CollapsingHeader(GetLang().ReadString("Objects", "obj_spawner_text", "Object Spawner")))
    {
        ImGui::TextWrapped(GetLang().ReadString("Objects", "obj_spawn_position", "Spawn Position (X | Y | Z)"));
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::InputFloat3("", &position.X);
        ImGui::PopItemWidth();
        if (ImGui::Button(GetLang().ReadString("Objects", "obj_get_player_pos", "Get Player Position"), { -FLT_MIN, 0 }))
        {
            if (player)
                position = *player->GetCharacterThing()->GetPosition();
        }

        ImGui::Text(GetLang().ReadString("Objects", "obj_name", "Object Name"));
        ImGui::SameLine();
        ShowHelpMarker(GetLang().ReadString("Objects", "obj_help", "Object list is available in Help menu."));
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::InputText("##object", objectName, sizeof(objectName));
        ImGui::PopItemWidth();

        if (ImGui::Button(GetLang().ReadString("Objects", "obj_get_id", "Get ID"), { -FLT_MIN, 0 }))
            objectId = GetThingID(objectName);

        ImGui::InputInt(GetLang().ReadString("Objects", "obj_id", "Object ID"), &objectId);

        if (objectId <= 0)
            ImGui::TextWrapped(GetLang().ReadString("Objects", "obj_invalid_id", "Invalid object ID!"));
        else
            if (ImGui::Button(GetLang().ReadString("Objects", "obj_create", "Create Object")))
                CreateThing(objectId, &position, 0, 0, 0, "newObj");
    }
    if (ImGui::CollapsingHeader(GetLang().ReadString("Objects", "obj_region_buildings", "Region Buildings")))
    {
        static std::vector<char> objectDataWindowsOpen;
        CThingSearchTools* searchTools = CMainGameComponent::Get()->GetWorld()->GetThingSearchTools();
        std::list<CThing*> regionBuildings = *searchTools->PeekTypeList(3);
        size_t buildingCount = regionBuildings.size();
        CTCHeroStats* heroStats = (CTCHeroStats*)player->GetCharacterThing()->GetTC(TCI_HERO_STATS);
        int shopsCount = 0;
        objectDataWindowsOpen.resize(buildingCount, false);

        for (auto building : regionBuildings)
        {
            if (building->HasTC(TCI_SHOP))
                shopsCount++;
        }
        ImGui::Text(GetLang().ReadString("Objects", "obj_shops_in_region", "Shops In Region: %d"), shopsCount);
        ImGui::Separator();
        if (ImGui::Button(GetLang().ReadString("Objects", "obj_unlock_doors", "Unlock All Doors")))
        {
            std::list<CThing*> allObjects = *searchTools->PeekTypeList(5);
            for (auto object : allObjects)
            {
                CTCDoor* door = (CTCDoor*)object->GetTC(TCI_DOOR);
                if (door)
                {
                    door->Open((CTCAnimationComplex*)1);
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(GetLang().ReadString("Objects", "obj_evict_residents", "Evict All Residents")))
        {
            for (auto building : regionBuildings)
            {
                CTCBuyableHouse* buyable = (CTCBuyableHouse*)building->GetTC(TCI_BUYABLE_HOUSE);

                if (buyable)
                {
                    buyable->Evict();
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(GetLang().ReadString("Objects", "obj_own_buildings", "Own All Buildings")))
        {
            for (auto building : regionBuildings)
            {
                if (building->HasTC(TCI_BUYABLE_HOUSE))
                {
                    CTCBuyableHouse* buyable = (CTCBuyableHouse*)building->GetTC(TCI_BUYABLE_HOUSE);
                    buyable->SetOwnedByPlayer(heroStats);
                }
            }
        }
        if (buildingCount != 0)
        {
            int i = 0;
            ImGui::BeginChild("##regionBuildingList", { 0, -ImGui::GetFrameHeightWithSpacing() + 400 }, true);
            for (CThing* object : regionBuildings)
            {
                CDefString* defName = object->GetDefName();
                CCharString buffer;
                CDefString::GetString(&buffer, defName->tablePos);
                char* charDefName = buffer.GetStringData();
                const char* thingName = (const char*)charDefName;

                std::string displayName = thingName;
                auto result = findValue(building_dir, thingName);
                if (!isPairEmpty(result))
                {
                    displayName = result.second.translate;
                }

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.15f, 0.75f, 0.35f, 1.0f));
                ImGui::LabelText("", displayName.c_str());
                ImGui::PopStyleColor();
                ImGui::SameLine();
                char* isOpen = &objectDataWindowsOpen[i];
                ImGui::PushID(i);
                if (ImGui::Button(GetLang().ReadString("Objects", "obj_open_data", "Open Data")))
                {
                    *isOpen = !(*isOpen);
                }
                ImGui::PopID();

                char windowTitle[256];
                snprintf(windowTitle, sizeof(windowTitle), "%s##%d", displayName.c_str(), i);

                if (*isOpen)
                {
                    FableMenu::DrawObjectData(windowTitle, object, (bool*)isOpen);
                }
                i++;
            }
            ImGui::EndChild();
        }
    }

    if (ImGui::CollapsingHeader(GetLang().ReadString("Objects", "obj_region_objects", "Region Objects")))
    {
        static std::vector<char> objectDataWindowsOpen;
        CThingSearchTools* searchTools = CMainGameComponent::Get()->GetWorld()->GetThingSearchTools();
        std::list<CThing*> allObjects = *searchTools->PeekTypeList(5);
        size_t objectCount = allObjects.size();
        objectDataWindowsOpen.resize(objectCount, false);
        static ImGuiTextFilter filter;
        static bool canTakeStockItems;
        if (ImGui::Checkbox(GetLang().ReadString("Objects", "obj_can_take_shop", "Can Take Shop Items"), &canTakeStockItems))
        {
            std::list<CThing*> stockItems;
            for (CThing* object : allObjects)
            {
                if (object->HasTC(TCI_STOCK_ITEM))
                    stockItems.push_back(object);
            }
            if (canTakeStockItems)
            {
                for (CThing* stockItem : stockItems)
                {
                    CCharString AddToInventory((char*)"CTCActionUsePutInInventory");
                    stockItem->AddTC(&AddToInventory, 0, 0);
                }
                Patch<char>(0x773538, 0x75);
            }
            else
            {
                for (CThing* stockItem : stockItems)
                {
                    if (stockItem->HasTC(TCI_ON_ACTION_USE))
                        stockItem->RemoveTC(TCI_ON_ACTION_USE);
                }
                Patch<char>(0x773538, 0x74);
            }
        }
        ImGui::Separator();
        ImGui::Text(GetLang().ReadString("Objects", "obj_list_search", "Search"));
        ImGui::PushItemWidth(-FLT_MIN);
        filter.Draw("##rolist");
        ImGui::PopItemWidth();
        ImGui::BeginChild("##regionObjectsList", { 0, -ImGui::GetFrameHeightWithSpacing() + 400 }, true);
        if (objectCount != 0)
        {
            int i = 0;
            for (CThing* object : allObjects)
            {
                CDefString* defName = object->GetDefName();
                CCharString buffer;
                CDefString::GetString(&buffer, defName->tablePos);
                char* charDefName = buffer.GetStringData();
                const char* thingName = (const char*)charDefName;

                std::string displayName = thingName;
                auto result = findValue(object_dir, thingName);
                if (!isPairEmpty(result))
                {
                    displayName = result.second.translate;
                }

                if (filter.PassFilter(thingName))
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.15f, 0.75f, 0.35f, 1.0f));
                    ImGui::LabelText("", displayName.c_str());
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    char* isOpen = &objectDataWindowsOpen[i];
                    ImGui::PushID(i);
                    if (ImGui::Button(GetLang().ReadString("Objects", "obj_open_data", "Open Data")))
                    {
                        *isOpen = !(*isOpen);
                    }
                    ImGui::PopID();

                    char windowTitle[256];
                    snprintf(windowTitle, sizeof(windowTitle), "%s##%d", displayName.c_str(), i);

                    if (*isOpen)
                    {
                        FableMenu::DrawObjectData(windowTitle, object, (bool*)isOpen);
                    }
                    ++i;
                }
            }
        }
        ImGui::EndChild();
    }
}

void FableMenu::DrawAppearanceCollapse(CThing* thing)
{
    static int alpha = 255;
    ImGui::SliderInt(GetLang().ReadString("Appearance", "ap_alpha", "Alpha"), &alpha, 0, 255);

    static ImVec4 appearanceColor = { 1.0, 1.0, 1.0, 1.0 };
    ImGui::ColorEdit3(GetLang().ReadString("Appearance", "ap_color", "Color"), (float*)&appearanceColor);

    static bool highlight = false;
    static ImVec4 highlightColor = { 1.0, 1.0, 1.0, 1.0 };

    ImGui::Checkbox(GetLang().ReadString("Appearance", "ap_change_highlight", "Change Highlight"), &highlight);

    if (highlight)
    {
        ImGui::ColorEdit4(GetLang().ReadString("Appearance", "ap_highlight", "Highlight"), (float*)&highlightColor);
    }

    CRGBAFloat acolor(appearanceColor.x, appearanceColor.y, appearanceColor.z, appearanceColor.w);
    CRGBAFloat hcolor(highlightColor.x, highlightColor.y, highlightColor.z, highlightColor.w);

    static float scale = 1.0f;

    CTCGraphicAppearance* ga = (CTCGraphicAppearance*)thing->GetTC(TCI_GRAPHIC_APPEARANCE_NEW);

    ImGui::InputFloat(GetLang().ReadString("Appearance", "ap_scale", "Scale"), &scale);

    if (ImGui::Button(GetLang().ReadString("Appearance", "ap_btn_update", "Update##Appearance"), { -FLT_MIN, 0 }))
    {
        if (ga)
        {
            ga->SetAlpha(alpha);
            ga->SetColor(&acolor.GetUINTColor(), ga);

            if (highlight)
                ga->SetAsHighlighted(5, 0, &hcolor.GetUINTColor(), 1, 1, MESH_EFFECT_PRIORITY_SHIELD_SPELL_SPECIAL_OVERRIDE, ga);

            ga->SetScale(scale);
        }
    }

    if (ImGui::Button(GetLang().ReadString("Appearance", "ap_btn_clear_highlight", "Clear Highlight"), { -FLT_MIN, 0 }))
    {
        ga->ClearHighlighted(ga);
    }

    CTCLight* light = (CTCLight*)thing->GetTC(TCI_LIGHT);

    if (light)
    {
        ImGui::Separator();
        ImGui::Text(GetLang().ReadString("Appearance", "ap_light", "Light"));
        ImGui::Separator();
        static float lightFlicker = 1.0f;
        static float innerRadius = 1.0f;
        static float outerRadius = 10.0f;
        static ImVec4 lightColor = { 1.0, 1.0, 1.0, 1.0 };

        if (ImGui::InputFloat(GetLang().ReadString("Appearance", "ap_inner_radius", "Inner Radius"), &innerRadius))
        {
            light->SetInnerRadius(innerRadius);
        }
        if(ImGui::InputFloat(GetLang().ReadString("Appearance", "ap_outer_radius", "Outer Radius"), &outerRadius))
        {
            light->SetOuterRadius(outerRadius);
        }
        if (ImGui::ColorEdit3(GetLang().ReadString("Appearance", "ap_light_color", "Light Color"), (float*)&lightColor))
        {
            CRGBAFloat lcolor = { lightColor.x, lightColor.y, lightColor.z, lightColor.w };
            light->SetColour(&lcolor.GetUINTColor());
        }
        if (ImGui::Checkbox(GetLang().ReadString("Appearance", "ap_enable_light", "Enable Light"), &light->m_bActive))
        {
            light->SetOverridden(1);
            light->SetActive(light->m_bActive);
        }
    }
}

void DrawAnimationCollapse(CThing* thing)
{
    static char animInputName[256] = {};
    static bool useInput;
    static int selectedAnimation;
    static bool stayOnLastFrame;
    static bool useMovement;
    static bool addAsQueuedAction;
    static bool waitForAnimToFinish;
    static bool usePhysics;
    static bool allowLooking;
    static bool looping;
    static int numLoops;
    static int animPriority;

    ImGui::Separator();
    ImGui::Text(GetLang().ReadString("Animation", "anim_text", "Animations"));
    ImGui::Separator();
    if (!useInput)
    {
        if (ImGui::BeginCombo("##animations", getById(creature_animations_dir, selectedAnimation).second.translate.c_str()))
        {
            const std::string selectID ="";
            for (auto& pair : creature_animations_dir)
            {
                int intKey = std::stoi(pair.first);
                bool isSelected = (selectedAnimation == intKey);

                if (ImGui::Selectable(pair.second.translate.c_str(), isSelected))
                {
                    selectedAnimation = intKey;
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
    else
    {
        ImGui::InputText("##anim", animInputName, sizeof(animInputName));
    }
    ImGui::SameLine();
    ImGui::Text(GetLang().ReadString("Animation", "anim_name", "Animation Name"));
    ImGui::Checkbox(GetLang().ReadString("Animation", "anim_manual_input", "Manual Input##anim"), &useInput);
    ImGui::SameLine();
    ImGui::Checkbox(GetLang().ReadString("Animation", "anim_looping", "Looping"), &looping);
    ImGui::SameLine();
    ImGui::Checkbox(GetLang().ReadString("Animation", "anim_use_physics", "Use Physics"), &usePhysics);
    ImGui::SameLine();
    ImGui::Checkbox(GetLang().ReadString("Animation", "anim_use_movement", "Use Movement"), &useMovement);
    ImGui::Checkbox(GetLang().ReadString("Animation", "anim_allow_looking", "Allow Looking"), &allowLooking);
    ImGui::SameLine();
    ImGui::Checkbox(GetLang().ReadString("Animation", "anim_stay_last_frame", "Stay In Last Frame"), &stayOnLastFrame);
    ImGui::SameLine();
    ImGui::Checkbox(GetLang().ReadString("Animation", "anim_wait_finish", "Wait For Anim To Finish"), &waitForAnimToFinish);
    ImGui::Separator();
    ImGui::InputInt(GetLang().ReadString("Animation", "anim_priority", "Animation Priority"), &animPriority);
    if (looping)
        ImGui::InputInt(GetLang().ReadString("Animation", "anim_num_loops", "Number Loops"), &numLoops);
    if (ImGui::Button(GetLang().ReadString("Animation", "anim_btn_play", "Play Animation"), { -FLT_MIN, 0 }))
    {
        CTCScriptedControl* scriptControl = (CTCScriptedControl*)thing->GetTC(TCI_SCRIPTED_CONTROL);
        CTCScriptedControl::CActionBase* animation = (CTCScriptedControl::CActionBase*)GameMalloc(180);

        std::string animationName;
        if (!useInput) {
            animationName = getById(creature_animations_dir, selectedAnimation).second.original;
        }
        else {
            animationName = animInputName;
        }

        Notifications->SetNotificationTime(2500);
        Notifications->PushNotification(animationName.c_str());
        CCharString name((char*)animationName.c_str());

        new CActionPlayAnimation(animation, &name, stayOnLastFrame, looping, numLoops, useMovement, animPriority, 0, waitForAnimToFinish, usePhysics, false, allowLooking);
        scriptControl->AddAction(animation);
    }
}

void FableMenu::DrawActionsCollapse(CThing* thing)
{
    DrawAnimationCollapse(thing);

    ImGui::Separator();
    ImGui::Text(GetLang().ReadString("Actions", "act_carrying", "Carrying"));
    ImGui::Separator();

    static bool destroyDropped = false;
    ImGui::Checkbox(GetLang().ReadString("Actions", "act_destroy_dropped", "Destroy Dropped Weapon"), &destroyDropped);
    if (ImGui::Button(GetLang().ReadString("Actions", "act_take_crate", "Take Crate")))
    {
        FableMenu::TakeActionItem(thing, (char*)"OBJECT_CRATE_SMALL_EXPLOSIVE_01_USABLE");
    }
    ImGui::SameLine();
    if (ImGui::Button(GetLang().ReadString("Actions", "act_drop_carried", "Drop Carried")))
    {
        // Patch to make it work for swords
        Patch<char>(0x845D86 + 1, 0x84);

        CCreatureAction_DropWeapon* drop = (CCreatureAction_DropWeapon*)GameMalloc(100);
        new CCreatureAction_DropWeapon(drop, thing);

        thing->SetCurrentAction((CTCBase*)drop);

        CTCCarrying* carrying = (CTCCarrying*)thing->GetTC(TCI_CARRYING);

        if (carrying)
        {
            CThing* primarySlotThing = carrying->GetThingInPrimarySlot();

            if (destroyDropped && primarySlotThing)
            {
                if (primarySlotThing->HasTC(TCI_INVENTORY_ITEM))
                {
                    CTCInventoryItem* item = (CTCInventoryItem*)primarySlotThing->GetTC(TCI_INVENTORY_ITEM);
                    item->RemoveFromInventory();
                }
            }
        }
    }
	ImGui::Separator();
    if (ImGui::Button(GetLang().ReadString("Actions", "act_finish_action", "Finish Current Action")))
    {
        thing->FinishCurrentAction();
    }
}

void FableMenu::DrawPhysicsCollapse(CThing* thing)
{
    ImGui::Separator();
    ImGui::Text(GetLang().ReadString("Physics", "phys_xyz", "X | Y | Z"));
    ImGui::Separator();
    CTCPhysicsStandard* physics = (CTCPhysicsStandard*)thing->GetTC(TCI_PHYSICS);
    ImGui::InputFloat3(GetLang().ReadString("Physics", "phys_position", "Position"), &physics->GetPosition()->X);
    if (thing->HasTC(TCI_HERO_STATS))
        ImGui::InputFloat3(GetLang().ReadString("Physics", "phys_velocity", "Velocity"), &physics->GetVelocity()->X);
    ImGui::Separator();
    ImGui::InputFloat3(GetLang().ReadString("Physics", "phys_forward", "Forward"), &physics->GetRHSet()->Forward.X);
    ImGui::InputFloat3(GetLang().ReadString("Physics", "phys_up", "Up"), &physics->GetRHSet()->Up.X);
    ImGui::Separator();

    bool isPhysicsEnabled = physics->IsPhysicsEnabled();

    if (ImGui::Checkbox(GetLang().ReadString("Physics", "phys_enable_phys", "Enable Physics"), &isPhysicsEnabled))
    {
        physics->EnablePhysics(isPhysicsEnabled);
    }
}

void FableMenu::DrawObjectData(const char* windowTitle, CThing* object, bool* isOpen)
{
    if (!object)
    {
        *isOpen = false;
    }
    if (*isOpen)
    {
        ImGui::SetNextWindowPos({ 700,200 }, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({ 600,400 }, ImGuiCond_FirstUseEver);
        if (ImGui::Begin(windowTitle, isOpen));
        {
            CTCBuyableHouse* buyableHouse = (CTCBuyableHouse*)object->GetTC(TCI_BUYABLE_HOUSE);

            ImGui::Separator();
            ImGui::Text(GetLang().ReadString("ObjData", "od_object", "Object"));
            ImGui::Separator();

            if (ImGui::Button(GetLang().ReadString("ObjData", "od_show", "Show")))
            {
                object->SetInLimbo(0);
            }
            ImGui::SameLine();
            if (ImGui::Button(GetLang().ReadString("ObjData", "od_hide", "Hide")))
            {
                object->SetInLimbo(1);
            }
            if (object->HasTC(TCI_STOCK_ITEM) || object->HasTC(TCI_HERO_RECEIVE_ITEMS))
            {
                if (ImGui::Button(GetLang().ReadString("ObjData", "od_add_inventory", "Add To Inventory")))
                {
                    CThing* playerCharacter = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer()->GetCharacterThing();
                    CCreatureAction_AddRealObjectToInventory* addToInventory = (CCreatureAction_AddRealObjectToInventory*)GameMalloc(180);
                    new CCreatureAction_AddRealObjectToInventory(addToInventory, playerCharacter, object);
                    playerCharacter->SetCurrentAction((CTCBase*)addToInventory);
                }
            }
            if (!buyableHouse)
            {
                if (ImGui::Button(GetLang().ReadString("ObjData", "od_teleport_to_player", "Teleport To Player Position")))
                {
                    CTCPhysicsStandard* objectPhysics = (CTCPhysicsStandard*)object->GetTC(TCI_PHYSICS);
                    CThing* playerCharacter = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer()->GetCharacterThing();
                    objectPhysics->SetPosition(playerCharacter->GetPosition());
                    objectPhysics->EnablePhysics(0);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(GetLang().ReadString("ObjData", "od_teleport_player_to", "Teleport Player to position")))
            {

                CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer();
                if (plr)
                {
                    CThing* t = plr->GetCharacterThing();
                    *t->GetPosition() = *object->GetPosition();
                }
            }
            CTCChest* chest = (CTCChest*)object->GetTC(TCI_CONTAINER);
            if (chest)
            {
                ImGui::Separator();
                ImGui::Text(GetLang().ReadString("ObjData", "od_chest", "Chest"));
                ImGui::Separator();

                if (ImGui::Button(GetLang().ReadString("ObjData", "od_open", "Open"), { -FLT_MIN, 0 }))
                {
                    chest->Open();
                }
                if (ImGui::Button(GetLang().ReadString("ObjData", "od_close", "Close"), { -FLT_MIN, 0 }))
                {
                    chest->Close();
                }
            }
            if (buyableHouse)
            {
                ImGui::Separator();
                ImGui::Text(GetLang().ReadString("ObjData", "od_house_features", "House Features"));
                ImGui::Separator();
                bool isUsed = buyableHouse->isBuildingBeingUsed(0);
                ImGui::Text(GetLang().ReadString("ObjData", "od_is_occupied", "Is Occupied: %s"), isUsed ? GetLang().ReadString("ObjData", "od_is_yes", "Yes") : GetLang().ReadString("ObjData", "od_is_no", "No"));

                if (ImGui::Button(GetLang().ReadString("ObjData", "od_remove_owner", "Remove Owner")))
                {
                    buyableHouse->Evict();
                }
                ImGui::SameLine();
                if (ImGui::Button(GetLang().ReadString("ObjData", "od_set_rented", "Set Rented")))
                {
                    buyableHouse->SetRented(1);
                }
                ImGui::SameLine();
                if (ImGui::Button(GetLang().ReadString("ObjData", "od_set_owned", "Set Owned By Player")))
                {
                    CThing* playerCharacter = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer()->GetCharacterThing();
                    CTCHeroStats* heroStats = (CTCHeroStats*)playerCharacter->GetTC(TCI_HERO_STATS);
                    if (heroStats)
                        buyableHouse->SetOwnedByPlayer(heroStats);
                }
            }
            ImGui::Separator();
            if (ImGui::CollapsingHeader(GetLang().ReadString("ObjData", "od_physics", "Physics")))
            {
                DrawPhysicsCollapse(object);
            }
            if (ImGui::CollapsingHeader(GetLang().ReadString("ObjData", "od_appearance", "Appearance")))
            {
                DrawAppearanceCollapse(object);
            }
            ImGui::End();
        }
    }
}

void FableMenu::DrawCreatureData(const char* windowTitle, CThing* creature, bool* isOpen)
{
    if (!creature)
    {
        *isOpen = false;
    }
    if (*isOpen)
    {
        ImGui::SetNextWindowPos({ 700,200 }, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({ 600,500 }, ImGuiCond_FirstUseEver);

        if (ImGui::Begin(windowTitle, isOpen));
        {
            ImGui::Separator();
            ImGui::Text(GetLang().ReadString("CrtData", "cd_data", "Data"));
            ImGui::Separator();

            if (ImGui::InputFloat(GetLang().ReadString("CrtData", "cd_health", "Health"), &creature->m_fHealth))
            {
                if (creature->m_fHealth > creature->m_fMaxHealth)
                    creature->m_fMaxHealth = creature->m_fHealth;
            }
            if (ImGui::Button(GetLang().ReadString("CrtData", "cd_kill", "Kill")))
            {
                creature->Kill(true);
                *isOpen = false;
            }
            ImGui::SameLine();
            if (ImGui::Button(GetLang().ReadString("CrtData", "cd_teleport_to_player", "Teleport To Player Position")))
            {
                CTCPhysicsStandard* creaturePhysics = (CTCPhysicsStandard*)creature->GetTC(TCI_PHYSICS);
                CThing* playerCharacter = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer()->GetCharacterThing();
                creaturePhysics->SetPosition(playerCharacter->GetPosition());
            }
            ImGui::SameLine();
            if (ImGui::Button(GetLang().ReadString("CrtData", "cd_teleport_player_to", "Teleport Player to position")))
            {

                CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer();
                if (plr)
                {
                    CThing* t = plr->GetCharacterThing();
                    *t->GetPosition() = *creature->GetPosition();
                }
            }
            if (ImGui::Button(GetLang().ReadString("CrtData", "cd_limbo", "Finish Current Action")))
            {
                creature->FinishCurrentAction();
                creature->ClearQueuedActions();
            }
            ImGui::Separator();
            ImGui::Text(GetLang().ReadString("Actions", "act_finish_action", "Limbo"));
            ImGui::Separator();
            if (ImGui::Button(GetLang().ReadString("CrtData", "cd_show", "Show")))
            {
                creature->SetInLimbo(0);
            }
            ImGui::SameLine();
            if (ImGui::Button(GetLang().ReadString("CrtData", "cd_hide", "Hide")))
            {
                creature->SetInLimbo(1);
            }
            ImGui::Separator();
            if (ImGui::CollapsingHeader(GetLang().ReadString("CrtData", "cd_behavior", "Behavior")))
            {
                ImGui::Separator();
                ImGui::Text(GetLang().ReadString("CrtData", "cd_brain", "Brain"));
                ImGui::Separator();
                static int brainID = 0;

                std::string currentDisplay = szBrainNames[brainID];
                auto result = findValue(brain_dir, szBrainNames[brainID]);
                if (!isPairEmpty(result))
                {
                    currentDisplay = result.second.translate;
                }

                if (ImGui::BeginCombo("##CreatureBrain", currentDisplay.c_str()))
                {
                    for (int i = 0; i < IM_ARRAYSIZE(szBrainNames); i++)
                    {
                        std::string displayText = szBrainNames[i];
                        auto result2 = findValue(brain_dir, szBrainNames[i]);
                        if (!isPairEmpty(result2))
                        {
                            displayText = result2.second.translate;
                        }

                        bool isSelected = (brainID == i);

                        if (ImGui::Selectable(displayText.c_str(), isSelected))
                        {
                            brainID = i;
                        }

                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine();
                ImGui::Text(GetLang().ReadString("CrtData", "cd_brain_name", "Brain Name"));

                if (ImGui::Button(GetLang().ReadString("CrtData", "cd_set_brain", "Set Brain"), { -FLT_MIN, 0 }))
                {
                    ImGui::Text(GetLang().ReadString("CrtData", "cd_brain", "Brain"));
                    ImGui::Separator();
                    CCharString brainName((char*)szBrainNames[brainID]);
                    CGameDefinitionManager* defManager = CGameDefinitionManager::GetDefinitionManager();

                    int brainIndex = defManager->GetDefGlobalIndexFromName(&brainName);

                    int brawlik[1];
                    brawlik[0] = 0;

                    defManager->GetOpinionPersonalitDef(brainIndex, brawlik);

                    creature->SetNewBrain(brawlik[0]);
                }

                ImGui::Separator();
                ImGui::Text(GetLang().ReadString("CrtData", "cd_combat", "Combat"));
                ImGui::Separator();

                static int attackStyleID = 0;

                std::string currentDisplayA = szAttackStyleNames[attackStyleID];
                auto resultA = findValue(attack_dir, szAttackStyleNames[attackStyleID]);
                if (!isPairEmpty(result))
                {
                    currentDisplayA = resultA.second.translate;
                }

                if (ImGui::BeginCombo("##CreatureAttackStyle", currentDisplayA.c_str()))
                {
                    for (int i = 0; i < IM_ARRAYSIZE(szAttackStyleNames); i++)
                    {
                        std::string displayText = szAttackStyleNames[i];
                        auto result2 = findValue(attack_dir, szAttackStyleNames[i]);
                        if (!isPairEmpty(result2))
                        {
                            displayText = result2.second.translate;
                        }

                        bool isSelected = (attackStyleID == i);

                        if (ImGui::Selectable(displayText.c_str(), isSelected))
                        {
                            attackStyleID = i;
                        }

                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine();
                ImGui::Text(GetLang().ReadString("CrtData", "cd_combat_name", "Combat Name"));

                if (ImGui::Button(GetLang().ReadString("CrtData", "cd_set_combat", "Set Combat"), { -FLT_MIN, 0 }))
                {
                    CGameDefinitionManager* defManager = CGameDefinitionManager::GetDefinitionManager();
                    CCharString combatName((char*)szAttackStyleNames[attackStyleID]);

                    int combatType = 0;
                    defManager->GetBrainDef(&combatName, &combatType);

                    CTCCombat* combat = (CTCCombat*)creature->GetTC(TCI_COMBAT);
                    combat->SetCombatType(&combatType);
                }

                if (creature->HasTC(TCI_ENEMY))
                {
                    ImGui::Separator();
                    ImGui::Text(GetLang().ReadString("CrtData", "cd_faction", "Faction"));
                    ImGui::Separator();
                    CThing* playerCharacter = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer()->GetCharacterThing();
                    CTCEnemy* enemy = (CTCEnemy*)creature->GetTC(TCI_ENEMY);

                    bool isEnemy = enemy->IsEnemyOf(playerCharacter);
                    ImGui::Text(GetLang().ReadString("CrtData", "cd_is_enemy", "Is Hero Enemy: %s"), isEnemy ? GetLang().ReadString("CrtData", "cd_is_yes", "Yes") : GetLang().ReadString("CrtData", "cd_is_no", "No"));

                    static int factionID = 0;

                    std::string currentDisplay = szFactions[factionID];
                    auto result = findValue(factions_dir, szFactions[factionID]);
                    if (!isPairEmpty(result))
                    {
                        currentDisplay = result.second.translate;
                    }

                    if (ImGui::BeginCombo("##CreatureFaction", currentDisplay.c_str()))
                    {
                        for (int i = 0; i < IM_ARRAYSIZE(szFactions); i++)
                        {
                            std::string displayText = szFactions[i];
                            auto result2 = findValue(factions_dir, szFactions[i]);
                            if (!isPairEmpty(result2))
                            {
                                displayText = result2.second.translate;
                            }

                            bool isSelected = (factionID == i);

                            if (ImGui::Selectable(displayText.c_str(), isSelected))
                            {
                                factionID = i;
                            }

                            if (isSelected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    ImGui::Text(GetLang().ReadString("CrtData", "cd_faction_name", "Faction Name"));
                    if (ImGui::Button(GetLang().ReadString("CrtData", "cd_set_faction", "Set Faction"), { -FLT_MIN, 0 }))
                    {
                        CCharString factionName((char*)szFactions[factionID]);
                        enemy->SetFaction(&factionName);
                    }
                }
            }
            if (ImGui::CollapsingHeader(GetLang().ReadString("Physics", "phys_text", "Physics")))
            {
                DrawPhysicsCollapse(creature);
            }
            if (ImGui::CollapsingHeader(GetLang().ReadString("CrtData", "cd_modes", "Modes")))
            {
                static int creatureModeID = 0;
                CTCCreatureModeManager* modeManager = (CTCCreatureModeManager*)creature->GetTC(TCI_ENTITY_MODE_MANAGER);

                std::string currentDisplay = "Unknown";
                auto result = findValue(creature_modes_dir, std::to_string(creatureModeID));
                if (!isPairEmpty(result))
                {
                    currentDisplay = result.second.translate;
                }

                if (ImGui::BeginCombo(GetLang().ReadString("CrtData", "cd_mode_name", "Mode Name"), currentDisplay.c_str()))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(szCreatureModeNames); n++)
                    {
                        std::string displayText = szCreatureModeNames[n];
                        auto result2 = findValue(creature_modes_dir, std::to_string(n));
                        if (!isPairEmpty(result2))
                        {
                            displayText = result2.second.translate;
                        }

                        bool is_selected = (creatureModeID == n);
                        if (ImGui::Selectable(displayText.c_str(), is_selected))
                            creatureModeID = n;
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                NCreatureMode::EMode creatureMode = (NCreatureMode::EMode)creatureModeID;

                if (ImGui::Button(GetLang().ReadString("CrtData", "cd_add_mode", "Add Mode")))
                {
                    modeManager->AddMode(creatureMode);
                }
                ImGui::SameLine();
                if (ImGui::Button(GetLang().ReadString("CrtData", "cd_remove_mode", "Remove Mode")))
                {
                    modeManager->RemoveMode(creatureMode);
                }
                if (ImGui::Button(GetLang().ReadString("CrtData", "cd_reset_modes", "Reset Modes"), { -FLT_MIN, 0 }))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(szCreatureModeNames); n++)
                    {
                        if (modeManager->IsModeActive((NCreatureMode::EMode)n))
                            modeManager->RemoveMode((NCreatureMode::EMode)n);
                    }
                    modeManager->SetupDefaultMode();
                }
            }

            bool hasAnimationTC = creature->HasTC(TCI_ANIMATION);

            if (!hasAnimationTC)
            {
                ImGui::BeginDisabled();
            }
            if (ImGui::CollapsingHeader(GetLang().ReadString("CrtData", "cd_animations", "Animations")))
            {
                DrawAnimationCollapse(creature);
            }
            if (!hasAnimationTC)
            {
                ImGui::EndDisabled();
            }

            bool hasCarryngTC = creature->HasTC(TCI_CARRYING);

            if (!hasCarryngTC)
            {
                ImGui::BeginDisabled();
            }

            if (ImGui::CollapsingHeader(GetLang().ReadString("CrtData", "cd_carrying", "Carrying")))
            {
                static int selectedCarrySlot;
                static int selectedWeaponID = 0;
                CTCCarrying* carry = (CTCCarrying*)creature->GetTC(TCI_CARRYING);
                bool isCarryWeapon = carry->IsCarryingWeapon(creature);

                ImGui::Text(GetLang().ReadString("CrtData", "cd_is_carrying", "Is Carrying Weapons: %s"), isCarryWeapon ? GetLang().ReadString("CrtData", "cd_is_yes", "Yes") : GetLang().ReadString("CrtData", "cd_is_no", "No"));

                std::string currentDisplay = szCreatureWeapons[selectedWeaponID];
                auto result = findValue(object_dir, szCreatureWeapons[selectedWeaponID]);
                if (!isPairEmpty(result))
                {
                    currentDisplay = result.second.translate;
                }

                if (ImGui::BeginCombo("##WeaponType", currentDisplay.c_str()))
                {
                    for (int i = 0; i < IM_ARRAYSIZE(szCreatureWeapons); i++)
                    {
                        std::string displayText = szCreatureWeapons[i];
                        auto result2 = findValue(object_dir, szCreatureWeapons[i]);
                        if (!isPairEmpty(result2))
                        {
                            displayText = result2.second.translate;
                        }

                        bool isSelected = (selectedWeaponID == i);

                        if (ImGui::Selectable(displayText.c_str(), isSelected))
                        {
                            selectedWeaponID = i;
                        }

                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::RadioButton(GetLang().ReadString("CrtData", "cd_right_hand", "Right Hand"), &selectedCarrySlot, 0);
                ImGui::SameLine();
                ImGui::RadioButton(GetLang().ReadString("CrtData", "cd_left_hand", "Left Hand"), &selectedCarrySlot, 1);
                ImGui::SameLine();
                ImGui::RadioButton(GetLang().ReadString("CrtData", "cd_both_hands", "Both Hands"), &selectedCarrySlot, 2);

                static const char* szCarrySlots[] = {
                    "CARRY_SLOT_RIGHT_HAND",
                    "CARRY_SLOT_LEFT_HAND"
                };

                if (ImGui::Button(GetLang().ReadString("CrtData", "cd_add_thing", "Add Thing")))
                {
                    CGameDefinitionManager* defManager = CGameDefinitionManager::GetDefinitionManager();
                    CCharString thingName((char*)szCreatureWeapons[selectedWeaponID]);
                    int thingIndex = defManager->GetDefGlobalIndexFromName(&thingName);
                    CThing* thing = CreateThing(thingIndex, creature->GetPosition(), 0, 0, 0, (char*)"obj");

                    if (selectedCarrySlot != 2)
                    {
                        CCharString slotName((char*)szCarrySlots[selectedCarrySlot]);
                        int index = defManager->GetDefGlobalIndexFromName(&slotName);
                        if (!carry->IsCarrySlotFree(index))
                        {
                            CThing* thingInSlot = carry->GetThingInCarrySlot(index);
                            carry->RemoveThingInCarrySlot(index, true);
                            thingInSlot->Kill(0);
                        }
                        carry->AddThingInCarrySlot(thing, index, true);
                    }
                    else
                    {
                        creature->ClearQueuedActions();
                        creature->FinishCurrentAction();
                        CCreatureAction_PickUpGenericBox* genericBox = (CCreatureAction_PickUpGenericBox*)malloc(180);
                        new CCreatureAction_PickUpGenericBox(genericBox, creature, thing);
                        creature->SetCurrentAction((CTCBase*)genericBox);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button(GetLang().ReadString("CrtData", "cd_remove_thing", "Remove Thing")))
                {
                    if (selectedCarrySlot != 2)
                    {
                        CGameDefinitionManager* defManager = CGameDefinitionManager::GetDefinitionManager();
                        CCharString slotName((char*)szCarrySlots[selectedCarrySlot]);
                        int index = defManager->GetDefGlobalIndexFromName(&slotName);
                        CThing* thingInSlot = carry->GetThingInCarrySlot(index);

                        if (!carry->IsCarrySlotFree(index))
                        {
                            carry->RemoveThingInCarrySlot(index, true);
                            thingInSlot->Kill(0);
                        }
                    }
                }
            }

            if (!hasCarryngTC)
            {
                ImGui::EndDisabled();
            }

            if (ImGui::CollapsingHeader(GetLang().ReadString("CrtData", "cd_wife", "Wife")))
            {
                CTCWife* wife = (CTCWife*)creature->GetTC(TCI_WIFE);

                if (ImGui::Button(GetLang().ReadString("CrtData", "cd_set_marryable", "Set As Marryable"), { -FLT_MIN, 0 }))
                {
                    if (!creature->HasTC(TCI_WIFE))
                    {
                        CCharString tcName((char*)"CTCWife");
                        creature->AddTC(&tcName, 0, 0);
                    }
                }

                if (!wife)
                {
                    ImGui::BeginDisabled();
                }

                if (ImGui::Button(GetLang().ReadString("CrtData", "cd_marry", "Marry"), { -FLT_MIN, 0 }))
                {
                    wife->Marry(0);
                }

                bool isSexDisabled = creature->IsChild() && wife;

                if (isSexDisabled)
                {
                    ImGui::BeginDisabled();
                }

                if (ImGui::Button(GetLang().ReadString("CrtData", "cd_have_sex", "Have Sex"), { -FLT_MIN, 0 }))
                {
                    wife->HaveSex();
                }

                if (isSexDisabled)
                {
                    ImGui::EndDisabled();
                }

                if (ImGui::Button(GetLang().ReadString("CrtData", "cd_divorce", "Divorce"), { -FLT_MIN, 0 }))
                {
                    wife->Divorce();
                }

                if (!wife)
                {
                    ImGui::EndDisabled();
                }
            }
            if (ImGui::CollapsingHeader(GetLang().ReadString("Appearance", "ap_text", "Appearance")))
            {
                DrawAppearanceCollapse(creature);
            }
            ImGui::End();
        }
    }
}

void FableMenu::DrawCameraTab()
{
    if (TheCamera)
    {
        ImGui::Checkbox(GetLang().ReadString("Camera", "cam_set_position", "Set Camera Position"), &m_bCustomCameraPos);
        ImGui::InputFloat3(GetLang().ReadString("Camera", "cam_xyz", "X | Y | Z"), &camPos.X);

        ImGui::Checkbox(GetLang().ReadString("Camera", "cam_set_fov", "Set FOV"), &m_bCustomCameraFOV);

        if (m_bCustomCameraFOV)
            ImGui::InputFloat(GetLang().ReadString("Camera", "cam_fov", "FOV"), &TheCamera->FOV);
        ImGui::Separator();
    }

    ImGui::Checkbox(GetLang().ReadString("Camera", "cam_free_camera", "Free Camera"), &ms_bFreeCam);
    if (ms_bFreeCam)
    {
        ImGui::Separator();
        if (!m_bCustomCameraPos)
            ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), GetLang().ReadString("Camera", "cam_warn_set_pos", "Check \"Set Camera Position\"!"));

        ImGui::Text(GetLang().ReadString("Camera", "cam_free_type", "Free Camera Type"));
        ImGui::Separator();
        ImGui::RadioButton(GetLang().ReadString("Camera", "cam_custom", "Custom (Recommended)"), &m_nFreeCameraMode, FREE_CAMERA_CUSTOM);
        ImGui::SameLine();
        ShowHelpMarker(GetLang().ReadString("Camera", "cam_custom_help", "A custom free camera implementation, uses NUMPAD keys by default to move the camera. Mouse and key settings can be changed in the Settings menu."));
        ImGui::RadioButton(GetLang().ReadString("Camera", "cam_original", "Original"), &m_nFreeCameraMode, FREE_CAMERA_ORIGINAL);
        if (m_nFreeCameraMode == FREE_CAMERA_CUSTOM)
        {
            ImGui::Separator();
            ImGui::InputFloat(GetLang().ReadString("Camera", "cam_speed", "Free Camera Speed"), &m_fFreeCamSpeed);
        }

        ImGui::Separator();
    }

    if (ImGui::Button(GetLang().ReadString("Camera", "cam_teleport_player", "Teleport Player To Camera Location"), { -FLT_MIN, 0 }))
    {
        CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer();
        if (plr)
        {
            CThing* t = plr->GetCharacterThing();
            *t->GetPosition() = TheCamera->pos;
        }
    }

}

void FableMenu::DrawWorldTab()
{
    CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer();
    CWorld* wrld = CMainGameComponent::Get()->GetWorld();
    if (wrld)
    {
        ImGui::Separator();
        ImGui::Text(GetLang().ReadString("World", "world_settings", "Settings"));
        ImGui::Separator();
        bool* minimap = wrld->GetMinimap();
        ImGui::Checkbox(GetLang().ReadString("World", "world_minimap", "Minimap"), minimap);
        if (plr)
        {
            bool& enemies = *(bool*)((int)plr + 0x21B);
            ImGui::Checkbox(GetLang().ReadString("World", "world_kill_mode", "Kill Mode"), &enemies);
        }
        ImGui::Checkbox(GetLang().ReadString("World", "world_enemy_god_mode", "Enemy God Mode"), &NGlobalConsole::EnemyGodMode);
        static bool fishingAnywhere;
        if (ImGui::Checkbox(GetLang().ReadString("World", "world_land_fishing", "Land Fishing"), &fishingAnywhere))
        {
            Patch(0x7F0210, { (unsigned char)((BYTE)fishingAnywhere + 0x74) });
        }
        ImGui::Separator();
    }
    if (wrld)
    {
        if (ImGui::CollapsingHeader(GetLang().ReadString("World", "world_time", "Time")))
        {
            int time = *(int*)((int)wrld + 28);
            if (time)
            {
                float& timeStep = *(float*)((int)time + 16);
                ImGui::SliderFloat(GetLang().ReadString("World", "world_time_step", "Time Step"), &timeStep, 0.001f, 1.0f);

                ImGui::Checkbox(GetLang().ReadString("World", "world_set_time", "Set Time"), &ms_bChangeTime);
                if (ms_bChangeTime)
                {
                    float& curTime = *(float*)((int)time + 8);
                    ImGui::SliderFloat(GetLang().ReadString("World", "world_time_value", "Time##set"), &curTime, 0.0, 1.0f);
                }
            }
        }
    }
    if (ImGui::CollapsingHeader(GetLang().ReadString("World", "world_region", "Region")))
    {
        static int hspID = 0;
        static char hspName[256] = {};
        static bool manualInput = false;

        bool& quest_regions = *(bool*)(0x1375741);
        ImGui::Checkbox(GetLang().ReadString("World", "world_quest_regions", "Quest Regions"), &quest_regions);
        static bool disableRegionBounds;
        if (ImGui::Checkbox(GetLang().ReadString("World", "world_disable_bounds", "Disable Region Bounds"), &disableRegionBounds))
        {
            if (disableRegionBounds)
            {
                Patch(0x81F3F6, { 0xB0, 0x01 });
            }
            else
            {
                Patch(0x81F3F6, { 0x8A, 0x44 });
            }
        }
        ImGui::Separator();
        if (m_bForceLoadRegion)
        {
            ImGui::BeginDisabled();
        }
        ImGui::Text(GetLang().ReadString("World", "world_hero_spawn", "Hero Spawn Point"));
        if (!manualInput)
        {
            std::string currentDisplay = szHolySites[hspID];
            auto result = findValue(holysites_dir, szHolySites[hspID]);
            if (!isPairEmpty(result))
            {
                currentDisplay = result.second.translate;
            }

            if (ImGui::BeginCombo("##hsplist", currentDisplay.c_str()))
            {
                for (int n = 0; n < IM_ARRAYSIZE(szHolySites); n++)
                {
                    std::string displayText = szHolySites[n];
                    auto result2 = findValue(holysites_dir, szHolySites[n]);
                    if (!isPairEmpty(result2))
                    {
                        displayText = result2.second.translate;
                    }

                    bool is_selected = (hspID == n);
                    if (ImGui::Selectable(displayText.c_str(), is_selected))
                        hspID = n;
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }
        }
        else
        {
            ImGui::InputText("##hspname", hspName, sizeof(hspName));
        }
        ImGui::Checkbox(GetLang().ReadString("World", "world_manual_input", "Manual Input"), &manualInput);
        if (ImGui::Button(GetLang().ReadString("World", "world_teleport", "Teleport"), { -FLT_MIN, 0 }))
        {
            if (ms_bSlowmotion)
            {
                wrld->GetBulletTime()->m_bActive = 0;
            }

            CCharString hsp_name(manualInput ? hspName : (char*)szHolySites[hspID]);
            wrld->TeleportHeroToHSP(&hsp_name);
        }
        if (m_bForceLoadRegion)
        {
            ImGui::EndDisabled();
        }
        if (ImGui::Button(GetLang().ReadString("World", "world_reload_region", "Reload Current Region"), { -FLT_MIN, 0 }))
        {
            CWorldMap* map = CThing::GetWorldMap();
            map->ReloadCurrentRegion();
        }
    }
    if (ImGui::CollapsingHeader(GetLang().ReadString("World", "world_particles", "Particles")))
    {
        ImGui::TextWrapped(GetLang().ReadString("World", "world_particles_help", "Particle list is available in Help menu."));
        static CVector particlePosition = {};
        static char particleName[512];
        static int attachType = -1;
        static bool isTemporaryParticle;
        static bool attachParticleToCamera;
        static bool particleNameError;

        ImGui::Checkbox(GetLang().ReadString("World", "world_enable_particles", "Enable Particles"), &NGlobalConsole::EnableParticles);
        ImGui::InputText(GetLang().ReadString("World", "world_particle_name", "Particle Name"), particleName, sizeof(particleName));
        ImGui::InputFloat3(GetLang().ReadString("World", "world_particle_position", "Particle Position"), &particlePosition.X);
        if (ImGui::Button(GetLang().ReadString("World", "world_particle_get_pos", "Get Player Position")))
        {
            CThing* playerCharacter = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer()->GetCharacterThing();
            particlePosition = *playerCharacter->GetPosition();
        }
        ImGui::RadioButton(GetLang().ReadString("World", "world_attach_default", "Default"), &attachType, -1);
        ImGui::SameLine();
        ImGui::RadioButton(GetLang().ReadString("World", "world_attach_camera", "Attach To Camera"), &attachType, 0);
        ImGui::SameLine();
        ImGui::RadioButton(GetLang().ReadString("World", "world_attach_body", "Attach To Body"), &attachType, 1);

        if (ms_bDisableCreateParticle)
            ImGui::BeginDisabled();

        if (ImGui::Button(GetLang().ReadString("World", "world_create_particle", "Create Particle"), { -FLT_MIN, 0 }))
        {
            CCharString ccstrParticle(particleName);
            CParticleEmitterDatabase* emitterDatabase = CParticleEmitterDatabase::Get();
            int partId = emitterDatabase->GetEmitterTemplateHandleFromName(&ccstrParticle);

            if (partId <= 0)
            {
                particleNameError = true;
            }
            else
            {
                if (particleNameError)
                {
                    particleNameError = false;
                }

                CThing* particleThing = CTCDParticleEmitter::Create(partId, &particlePosition, false);
                m_vCreatedParticles.push_back(particleThing);

                if (attachType != -1)
                {
                    CTCDParticleEmitter* particleEmitter = (CTCDParticleEmitter*)particleThing->GetTC(TCI_PARTICLE_EMITTER);

                    if(attachType == 0)
                    {
                        particleEmitter->AttachToCamera(8, 0.0);
                    }
                    else if (attachType == 1)
                    {
                        CThing* playerCharacter = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer()->GetCharacterThing();
                        CCharString name("body");
                        particleEmitter->AttachToThing(playerCharacter, 10, &name, 0, 0.0);
                    }

                    m_vAttachedParticles.push_back(particleThing);
                }
            }
        }

        if (particleNameError)
        {
            ImGui::Text(GetLang().ReadString("World", "world_particle_error", "Error: Undefined particle name"));
        }

        if (ms_bDisableCreateParticle)
            ImGui::EndDisabled();

        if (ImGui::Button(GetLang().ReadString("World", "world_clear_attachments", "Clear Attachments"), { 140, 25 }))
        {
            for (auto attachedParticle : m_vAttachedParticles)
            {
                if (attachedParticle->isThingAlive())
                {
                    if (attachedParticle->HasTC(TCI_PARTICLE_EMITTER))
                    {
                        CTCDParticleEmitter* attached = (CTCDParticleEmitter*)attachedParticle->GetTC(TCI_PARTICLE_EMITTER);
                        attached->ClearAttachments();
                    }
                }
            }
            m_vAttachedParticles.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button(GetLang().ReadString("World", "world_destroy_particles", "Destroy All Created Particles"), { 260, 25 }))
        {
            for (auto thing : m_vCreatedParticles)
            {
                if (thing->isThingAlive())
                {
                    thing->Kill(false);
                }
            }
            m_vCreatedParticles.clear();
            m_vAttachedParticles.clear();
        }
    }
}

void FableMenu::DrawQuestTab()
{
    if (!CQuestManager::Get())
        return;


    CQuestManager* q = CQuestManager::Get();

    ImGui::TextWrapped(GetLang().ReadString("Quest", "quest_note", "NOTE: Quest changes might break your savegame! To be safe, do any quest changes on a backup/alternative save."));

    static char scriptName[256] = {};
    if (ImGui::CollapsingHeader(GetLang().ReadString("Quest", "quest_status_control", "Status Control")))
    {
        ImGui::Text(GetLang().ReadString("Quest", "quest_name", "Quest Name"));
        static bool writeName = false;

        ImGui::PushItemWidth(-FLT_MIN);

        if (!writeName)
        {
            if (ImGui::BeginCombo("#qststatus", scriptName))
            {
                for (int n = 0; n < IM_ARRAYSIZE(szBuiltInQuests); n++)
                {
                    bool is_selected = (scriptName == szBuiltInQuests[n]);
                    if (ImGui::Selectable(szBuiltInQuests[n], is_selected))
                        sprintf(scriptName, szBuiltInQuests[n]);
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            ImGui::InputText("##quest", scriptName, sizeof(scriptName));
        }
        ImGui::Checkbox(GetLang().ReadString("Quest", "quest_manual_input", "Manual Input"), &writeName);

        ImGui::PopItemWidth();

        if (strlen(scriptName) > 0)
        {
            if (ImGui::Button(GetLang().ReadString("Quest", "quest_activate", "Activate"), { -FLT_MIN, 0 }))
            {
                CCharString str(scriptName);
                if (q->IsQuestActive(&str))
                {
                    Notifications->SetNotificationTime(2500);
                    Notifications->PushNotification("Quest \"%s\" is already active!", scriptName);
                }
                else
                {
                    if (q->ActivateQuest(&str, false, true))
                    {
                        Notifications->SetNotificationTime(2500);
                        Notifications->PushNotification("Quest \"%s\" activated!", scriptName);
                    }
                    else
                    {
                        Notifications->SetNotificationTime(2500);
                        Notifications->PushNotification("Failed to activate quest \"%s\"!", scriptName);
                    }
                }

            }
            if (ImGui::Button(GetLang().ReadString("Quest", "quest_deactivate", "Deactivate"), { -FLT_MIN, 0 }))
            {
                CCharString str(scriptName);
                if (!q->IsQuestActive(&str))
                {
                    Notifications->SetNotificationTime(2500);
                    Notifications->PushNotification("Quest \"%s\" is not active!", scriptName);
                }
                else
                {
                    q->DeactivateQuest(&str, 0);
                    Notifications->SetNotificationTime(2500);
                    Notifications->PushNotification("Quest \"%s\" deactivated!", scriptName);
                }
            }
        }
    }
    if (ImGui::CollapsingHeader(GetLang().ReadString("Tweaks", "quest_active", "Active Quests")))
    {
        static bool coreQuest = false;
        static bool optionalQuests = false;
        static bool scriptQuests = false;
        static bool localizedQuests = false;
        int activeQuests = 0;

        ImGui::Checkbox(GetLang().ReadString("Tweaks", "quest_core", "Core Quests"), &coreQuest);
        ImGui::SameLine();
        ImGui::Checkbox(GetLang().ReadString("Tweaks", "quest_optional", "Optional Quests"), &optionalQuests);
        ImGui::SameLine();
        ImGui::Checkbox(GetLang().ReadString("Tweaks", "quest_scripts", "Scripts"), &scriptQuests);
        ImGui::SameLine();
        ImGui::Checkbox(GetLang().ReadString("Tweaks", "quest_localize", "Localize Names"), &localizedQuests);

        ImGui::BeginChild("#qlist", { 0, -ImGui::GetFrameHeightWithSpacing() }, true);
        for (int i = 0; i < IM_ARRAYSIZE(szBuiltInQuests); i++)
        {
            CCharString quest_name((char*)szBuiltInQuests[i]);
            const char* localizedName = "NONAME_QUEST";

            if (q->IsQuestActive(&quest_name))
            {
                activeQuests++;

                CThing* cardThing = q->GetActiveQuestCardFromScriptName(&quest_name);
                bool isScript = !cardThing;

                if (isScript && !scriptQuests)
                {
                    continue;
                }

                if (!isScript)
                {
                    CTCQuestCard* card = (CTCQuestCard*)cardThing->GetTC(TCI_QUEST_CARD);

                    if (localizedQuests)
                    {
                        CWideString name;
                        card->GetQuestName(&name);
                        localizedName = GetUTF8String(name.GetWideStringData());
                    }

                    if (coreQuest || optionalQuests)
                    {
                        bool isCore = (coreQuest && card->IsRouteToAppearOnMinimap());
                        bool isOptional = (optionalQuests && card->IsOptional());

                        if (!isCore && !isOptional)
                        {
                            continue;
                        }
                    }
                }

                if (localizedQuests && !isScript)
                {
                    if (strcmp(localizedName, "NONAME_QUEST") == 0)
                    {
                        continue;
                    }

                    ImGui::LabelText("", localizedName);
                }
                else
                {
                    ImGui::LabelText("", szBuiltInQuests[i]);
                }
                ImGui::SameLine();
                ImGui::PushID(i);
                if (ImGui::Button(GetLang().ReadString("Tweaks", "quest_btn_deactivate", "Deactivate")))
                {
                    q->DeactivateQuest(&quest_name, 0);

                }
                if (cardThing)
                {
                    ImGui::SameLine();
                    if (ImGui::Button(GetLang().ReadString("Tweaks", "quest_btn_complete", "Complete")))
                    {
                        CTCQuestCard* card = (CTCQuestCard*)cardThing->GetTC(TCI_QUEST_CARD);
                        q->SetQuestAsCompleted(&quest_name, 0, 0, 0);
                    }
                }
                ImGui::PopID();
            }
        }
		ImGui::EndChild();
#ifdef _DEBUG
        if (activeQuests > 0)
        {
            if (ImGui::Button(GetLang().ReadString("Tweaks", "quest_deactivate_all", "Deactivate All"), { -FLT_MIN, 0 }))
            {
                q->DeactivateAllQuests();
                Notifications->SetNotificationTime(2500);
                Notifications->PushNotification(GetLang().ReadString("Tweaks", "quest_all_deactivated", "Все квесты отключены! (%d)"), activeQuests);
                activeQuests = 0;
            }
        }
#endif
    }

    if (ImGui::CollapsingHeader(GetLang().ReadString("Tweaks", "tweaks_text", "Tweaks")))
    {
        ImGui::Checkbox(GetLang().ReadString("Tweaks", "quest_no_bg_limit", "No Bodyguards Limit"), &m_bNoBodyGuardsLimit);
        ImGui::SameLine(); ShowHelpMarker(GetLang().ReadString("Tweaks", "quest_bg_help", "Allows to hire all bodyguards. Default limit is 2."));
        ImGui::Checkbox(GetLang().ReadString("Tweaks", "quest_lock_region", "Locking Leave Quest Region"), &NGlobalConsole::GEnableRegionLockingSaveSystem);
    }
}

void FableMenu::DrawMiscTab()
{
    CWorld* wrld = CMainGameComponent::Get()->GetWorld();
    ImGui::Separator();
    ImGui::Text(GetLang().ReadString("Misc", "misc_time_header", "Time"));
    ImGui::Separator();

    if (wrld)
    {
        CBulletTimeManager* time = wrld->GetBulletTime();
        if (time)
        {
            if (ImGui::Checkbox(GetLang().ReadString("Misc", "misc_slowmotion", "Slowmotion"), &time->m_bActive))
            {
                ms_bSlowmotion = time->m_bActive;
            }
        }
    }

    ImGui::Checkbox(GetLang().ReadString("Misc", "misc_update_ai", "Update AI"), &NGlobalConsole::EnableUpdateAI);
    ImGui::Checkbox(GetLang().ReadString("Misc", "misc_update_objects", "Update Objects"), &NGlobalConsole::EnableUpdateObjects);

    static bool creatureDecay = 1;
    static bool enableShortMelee = 0;
    if (ImGui::Checkbox(GetLang().ReadString("Misc", "misc_creature_decay", "Dead Creature Decay"), &creatureDecay))
    {
        Patch<char>(0x8362EA + 1, creatureDecay + 0x84);
    }
    ImGui::Separator();
    ImGui::Text(GetLang().ReadString("Misc", "misc_hero", "Hero"));
    ImGui::Separator();

    ImGui::Checkbox(GetLang().ReadString("Misc", "misc_hero_god", "Hero God Mode"), &NGlobalConsole::HeroGodMode);
    ImGui::Checkbox(GetLang().ReadString("Misc", "misc_hero_jump", "Enable Hero Jump"), &NGlobalConsole::EnableHeroJump);
    char jumpDesc[256];
    sprintf(jumpDesc, GetLang().ReadString("Misc", "misc_help_jump", "Jump action assigned to \"%s\" button. Jump key and others can be changed in settings menu."), eKeyboardMan::KeyToString(SettingsMgr->iHeroJumpKey));
    ImGui::SameLine(); ShowHelpMarker(jumpDesc);
    ImGui::Checkbox(GetLang().ReadString("Misc", "misc_hero_sprint", "Enable Hero Sprint"), &NGlobalConsole::EnableHeroSprint);

    ImGui::Separator();
    ImGui::Text(GetLang().ReadString("Misc", "misc_display", "Display"));
    ImGui::Separator();

    ImGui::Checkbox(GetLang().ReadString("Misc", "misc_display_hud", "Display HUD"), &GetHud()->m_bDisplay);
    if (ImGui::Checkbox(GetLang().ReadString("Misc", "misc_hide_autosave", "Hide Auto Save Progress"), &FGlobals::GDoNotCallStartAutoSaveProgress))
    {
        // Patch SaveGameState variable reset
        Patch(0x4A073B, { (BYTE)FGlobals::GDoNotCallStartAutoSaveProgress });
    }

    ImGui::Separator();
    ImGui::Text(GetLang().ReadString("Misc", "misc_cheats", "Cheats"));
    ImGui::Separator();
    ImGui::Checkbox(GetLang().ReadString("Misc", "misc_inf_health", "Infinite Health"), &m_bGodMode);
    ImGui::Checkbox(GetLang().ReadString("Misc", "misc_inf_will", "Infinite Will"), &m_bInfiniteWill);

    ImGui::Separator();
    ImGui::Text(GetLang().ReadString("Misc", "misc_console", "Console"));
    ImGui::Separator();
    ImGui::InputFloat(GetLang().ReadString("Misc", "misc_trading_mult", "Trading Price Multiplier"), CTCAIScratchPad::TradingPriceMult);

    if (ImGui::InputInt(GetLang().ReadString("Misc", "misc_fade_dist", "Primitive Fade Distance"), &NGlobalConsole::PrimitiveFadeDistance))
    {
        NGlobalConsole::ForcePrimitiveFadeDistance = (NGlobalConsole::PrimitiveFadeDistance > 0);
    }
    ImGui::InputFloat(GetLang().ReadString("Misc", "misc_speed_mult", "Override Speed Multiplier"), &NGlobalConsole::ConsoleOverrideMultiplier);
#ifdef _DEBUG
    ImGui::Checkbox("Debug Stress Test", &NGlobalConsole::GCombatStressTestDebug);
    ImGui::Separator();
    ImGui::Text("Debug");
    ImGui::Separator();
    if (TheCamera)
    {
        ImGui::Text("Camera: 0x%X", TheCamera);
    }
    if (wrld)
    {
        ImGui::Text("World Pointer: 0x%X", wrld);
        ImGui::Text("Game Component Pointer: 0x%X", CMainGameComponent::Get());
        ImGui::Text("Weather Settings: 0x%X", WeatherSettings);
    }

    CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer();
    if (plr)
    {
        CThing* t = plr->GetCharacterThing();
        ImGui::Text("Player Stats: 0x%X", t->GetTC(TCI_HERO_STATS));
        ImGui::Text("Player Morph: 0x%X\n", t->GetTC(TCI_APPEARANCE_MORPH));
        ImGui::Text("Player Experience: 0x%X\n", t->GetTC(TCI_HERO_EXPERIENCE));
        ImGui::Text("Player Thing: 0x%X\n", t);
        ImGui::Text("Player Physics: 0x%X\n", t->GetTC(TCI_PHYSICS));
        ImGui::Text("Player: 0x%X\n", plr);
        ImGui::Text("Draw: 0x%X\n", t->GetTC(TCI_GRAPHIC_APPEARANCE_NEW));
        ImGui::Text("Script Manager: 0x%X\n", wrld->GetScriptInfoManager());
    }
#endif
}

void FableMenu::DrawSettings()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.5f, 0.5f });
    ImGui::SetNextWindowPos({ ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f }, ImGuiCond_Once, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 700,700 }, ImGuiCond_Once);
    ImGui::Begin(GetLang().ReadString("Settings", "stn_text", "Settings"), &m_bSubmenuActive[SM_Settings]);

    static int settingID = 0;
    static const char* settingNames[] = {
        GetLang().ReadString("Settings","stn_menu","Menu"), 
		GetLang().ReadString("Settings","stn_ini","INI"),
		GetLang().ReadString("Settings","stn_keys","Keys"),
		GetLang().ReadString("Settings","stn_mouse","Mouse")
    };

    enum eSettings {
        MENU,
        INI,
        KEYS,
        MOUSE
    };

    ImGui::BeginChild("##settings", { 12 * ImGui::GetFontSize(), 0 }, true);

    for (int n = 0; n < IM_ARRAYSIZE(settingNames); n++)
    {
        bool is_selected = (settingID == n);
        if (ImGui::Selectable(settingNames[n], is_selected))
            settingID = n;
        if (is_selected)
            ImGui::SetItemDefaultFocus();
    }

    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##content", { 0, -ImGui::GetFrameHeightWithSpacing() });

    switch (settingID)
    {
    case MENU:
        ImGui::TextWrapped(GetLang().ReadString("MenuMain", "stn_menu_text", "All user settings are saved to fablemenu_user.ini."));
        ImGui::Text(GetLang().ReadString("MenuMain", "stn_menu_scale", "Menu Scale"));
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::InputFloat("##", &SettingsMgr->fMenuScale);
        ImGui::PopItemWidth();
        break;
    case INI:
        ImGui::TextWrapped(GetLang().ReadString("INI", "stn_ini_text", "These settings control FableMenu.ini options. Any changes require game restart to take effect."));
        ImGui::LabelText("##", GetLang().ReadString("INI", "stn_ini_core", "Core"));
        ImGui::Separator();
        ImGui::Checkbox(GetLang().ReadString("INI", "stn_ini_slowmotion", "Slowmotion Spell/Effect Affects Everything"), &SettingsMgr->bSlowMotionEffectsEverything);
        ImGui::Checkbox(GetLang().ReadString("INI", "stn_ini_windowed", "Windowed Mode"), &SettingsMgr->bUseBuiltInWindowedMode);
        ImGui::Separator();

        break;
    case KEYS:
        if (m_bPressingKey)
            ImGui::TextColored(ImVec4(0.f, 1.f, 0.3f, 1.f), GetLang().ReadString("Keys", "keys_press_key", "Press a key!"));

        if (ImGui::Button(GetLang().ReadString("Keys", "keys_reset_btn", "Reset Keys"), { -FLT_MIN, 0 }))
        {
            SettingsMgr->ResetKeys();
            Notifications->SetNotificationTime(2500);
            Notifications->PushNotification("Keys reset! Remember to save.");
        }

        ImGui::Separator();
        ImGui::LabelText("##", GetLang().ReadString("Keys", "keys_core", "Core"));
        ImGui::Separator();
        KeyBind(&SettingsMgr->iMenuOpenKey, GetLang().ReadString("Keys", "keys_open_close", "Open/Close Menu"), "menu");
        ImGui::Separator();
        ImGui::LabelText("##", GetLang().ReadString("Keys", "keys_camera", "Camera"));
        ImGui::Separator();

        KeyBind(&SettingsMgr->iFreeCameraKeyForward, GetLang().ReadString("Keys", "keys_forward", "Forward"), "x_plus");
        KeyBind(&SettingsMgr->iFreeCameraKeyBack, GetLang().ReadString("Keys", "keys_back", "Bacj"), "x_minus");
        KeyBind(&SettingsMgr->iFreeCameraKeyLeft, GetLang().ReadString("Keys", "keys_left", "Left"), "y_plus");
        KeyBind(&SettingsMgr->iFreeCameraKeyRight, GetLang().ReadString("Keys", "keys_right", "Right"), "y_minus");
        KeyBind(&SettingsMgr->iFreeCameraKeyUp, GetLang().ReadString("Keys", "keys_up", "Up"), "z_plus");
        KeyBind(&SettingsMgr->iFreeCameraKeyDown, GetLang().ReadString("Keys", "keys_down", "Down"), "z_minus");
        
        ImGui::Separator();
        ImGui::LabelText("##", GetLang().ReadString("Keys", "keys_actions_keys", "Actions Keys"));
        ImGui::Separator();
        GameKeyBind(&SettingsMgr->iHeroJumpKey, GetLang().ReadString("Keys", "keys_jump", "Jump"), "jump_action", GAME_ACTION_JUMP);
        ImGui::Separator();

        if (m_bPressingKey)
        {
            eVKKeyCode result = eKeyboardMan::GetLastKey();

            if (result >= VK_BACKSPACE && result < VK_KEY_NONE)
            {
                *m_pCurrentVarToChange = result;
                m_bPressingKey = false;
            }
        }
        break;
    case MOUSE:
        ImGui::TextWrapped(GetLang().ReadString("MenuMain", "stn_menu_text", "All user settings are saved to fablemenu_user.ini."));
        ImGui::Text(GetLang().ReadString("Mouse", "mouse_sensitivity", "Sensitivity"));
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::SliderFloat("##slider_mouse", &SettingsMgr->mouse.sens, 0, 10.0f);
        ImGui::PopItemWidth();
        ImGui::Checkbox(GetLang().ReadString("Mouse", "mouse_invert_x", "Invert X"), &SettingsMgr->mouse.invert_x);
        ImGui::Checkbox(GetLang().ReadString("Mouse", "mouse_invert_y", "Invert Y"), &SettingsMgr->mouse.invert_y);
        break;
    default:
        break;
    }

    if (ImGui::Button(GetLang().ReadString("Settings", "stn_btn_save", "Save"), { -FLT_MIN, 0 }))
    {
        Notifications->SetNotificationTime(2500);
        Notifications->PushNotification(GetLang().ReadString("Settings", "stn_save_notification", "Settings saved to FableMenu.ini and fablemenu_user.ini!"));
        GUIImplementationDX9::RequestFontReload();
        SettingsMgr->SaveSettings();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::End();
}

void FableMenu::DrawCreatureList()
{
    std::string selectID = "";

    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.5f, 0.5f });
    ImGui::SetNextWindowPos({ ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f }, ImGuiCond_Once, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 700,700 }, ImGuiCond_Once);
    ImGui::Begin(GetLang().ReadString("Creatures", "crt_list_text", "Creatures List"), &m_bSubmenuActive[SM_Creature_List]);

    static ImGuiTextFilter filter;
    ImGui::TextWrapped(GetLang().ReadString("Creatures", "crt_list_text_copy", "Click on any entry to copy to clipboard."));
    ImGui::Text(GetLang().ReadString("Creatures", "crt_list_search", "Search"));
    ImGui::PushItemWidth(-FLT_MIN);
    filter.Draw("");
    ImGui::PopItemWidth();

    ImGui::BeginChild("##wclist", { 0, -ImGui::GetFrameHeightWithSpacing() }, true);

    for (auto& pair : crt_dir)
    {
        const std::string key = pair.first;
		const std::string value = pair.second.original;
        const std::string display_text = pair.second.translate + " [" + pair.second.original + "]";

        if (filter.PassFilter(display_text.c_str()))
        {
            bool is_selected = (selectID == key);
            if (ImGui::Selectable(display_text.c_str(), is_selected))
            {
                selectID = key;

                char name[256] = {};
				char description[512] = {};
                sprintf(name, "%s", value.c_str());
                sprintf(description, "%s", display_text.c_str());
                CopyToClipboard(name, description);
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::End();
}

void FableMenu::DrawObjectList()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.5f, 0.5f });
    ImGui::SetNextWindowPos({ ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f }, ImGuiCond_Once, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 700,700 }, ImGuiCond_Once);
    ImGui::Begin(GetLang().ReadString("Objects", "obj_list_text", "Object List"), &m_bSubmenuActive[SM_Object_List]);

    static ImGuiTextFilter filter;
    ImGui::TextWrapped(GetLang().ReadString("Lists", "list_copy_hint", "Click on any entry to copy to clipboard."));
    ImGui::Text(GetLang().ReadString("Objects", "obj_list_search", "Search"));
    ImGui::PushItemWidth(-FLT_MIN);
    filter.Draw("##wolist");
    ImGui::PopItemWidth();

    ImGui::BeginChild("##olist", { 0, -ImGui::GetFrameHeightWithSpacing() }, true);

    std::string selectID = "";
    for (auto& pair : object_dir)
    {
        const std::string key = pair.first;
        const std::string value = pair.second.original;
        const std::string display_text = pair.second.translate + " [" + pair.second.original + "]";

        if (filter.PassFilter(display_text.c_str()))
        {
            bool is_selected = (selectID == key);
            if (ImGui::Selectable(display_text.c_str(), is_selected))
            {
                selectID = key;

                char name[256] = {};
                char description[512] = {};
                sprintf(name, "%s", value.c_str());
                sprintf(description, "%s", display_text.c_str());
                CopyToClipboard(name, description);
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::End();
}

void FableMenu::DrawParticleList()
{
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.5f, 0.5f });
    ImGui::SetNextWindowPos({ ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f }, ImGuiCond_Once, { 0.5f, 0.5f });
    ImGui::SetNextWindowSize({ 700,700 }, ImGuiCond_Once);
    ImGui::Begin(GetLang().ReadString("Particles", "part_list_text", "Particle List"), &m_bSubmenuActive[SM_Particle_List]);

    static ImGuiTextFilter filter;
    ImGui::TextWrapped(GetLang().ReadString("Lists", "list_copy_hint", "Click on any entry to copy to clipboard."));
    ImGui::Text(GetLang().ReadString("Particles", "part_list_search", "Search"));
    ImGui::PushItemWidth(-FLT_MIN);
    filter.Draw("##wplist");
    ImGui::PopItemWidth();

    ImGui::BeginChild("##plist", { 0, -ImGui::GetFrameHeightWithSpacing() }, true);

    static int selectID = 0;
    for (int n = 0; n < IM_ARRAYSIZE(szParticleList); n++)
    {
        if (filter.PassFilter(szParticleList[n]))
        {
            bool is_selected = (selectID == n);
            if (ImGui::Selectable(szParticleList[n], is_selected))
            {
                selectID = n;

                char name[256] = {};
                sprintf(name, "%s", szParticleList[selectID]);
                CopyToClipboard(name);
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::End();
}

void FableMenu::DrawKeyBind(char* name, int* var)
{
    ImGui::SameLine();

    static char butName[256] = {};
    sprintf(butName, "%s##key%s", eKeyboardMan::KeyToString(*var), name);
    if (ImGui::Button(butName))
    {
        m_bPressingKey = true;
        m_pCurrentVarToChange = var;
    }
}

void FableMenu::GameKeyBind(int* var, char* bindName, char* name, EGameAction action)
{
    CUserProfileManager* profile = CUserProfileManager::Get();
    CActionInputControl input;
    profile->GetAssignedInputForAction(action, !FGlobals::GUsePassiveAggressiveMode, &input);

    if (gameKeyCodes[input.KeyboardKey] != *var)
    {
        EInputKey key = eKeyboardMan::GetInputFromVKKeyCode(*var);
        
        if (key)
        {
            profile->SetAssignedInputKeyboard(action, key, !FGlobals::GUsePassiveAggressiveMode);
        }
    }
    else
    {
        *var = gameKeyCodes[input.KeyboardKey];
    }

    ImGui::LabelText("", bindName);
    DrawKeyBind(name, var);
}

void FableMenu::KeyBind(int* var, char* bindName, char* name)
{
    ImGui::LabelText("", bindName);
    DrawKeyBind(name, var);
}

void HookWorldUpdate()
{
    if (!InGame())
        return;

    CWorld* wrld = CMainGameComponent::Get()->GetWorld();
    if (wrld)
    {
        CBulletTimeManager* time = wrld->GetBulletTime();
        if (wrld->isLoadRegion())
        {
            for (CThing* thing : FableMenu::m_vAttachedParticles)
            {
                if (thing->isThingAlive())
                {
                    CTCDParticleEmitter* attached = (CTCDParticleEmitter*)thing->GetTC(TCI_PARTICLE_EMITTER);
                    attached->ClearAttachments();
                }
            }
            FableMenu::m_vAttachedParticles.clear();
            FableMenu::m_vCreatedParticles.clear();

            if (FableMenu::ms_bSlowmotion && time->m_bActive)
                time->m_bActive = 0;
        }
        else
        {
            if (FableMenu::ms_bSlowmotion && !time->m_bActive)
				time->m_bActive = 1;
        }

        CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetMainPlayer();
        if (plr)
        {
            CThing* t = plr->GetCharacterThing();

            if (GetMenu().m_bIsActive && !GetMenu().m_bFrozeControls)
            {
                plr->DisableInput();
				GetMenu().m_bFrozeControls = true;
            }
            else if (!GetMenu().m_bIsActive && GetMenu().m_bFrozeControls)
            {
                plr->EnableInput();
				GetMenu().m_bFrozeControls = false;
            }

            if (GetMenu().m_bGodMode)
            {
                CThing* t = plr->GetCharacterThing();
                if (t)
                    t->m_fHealth = 1000.0f;
            }

            if (GetMenu().m_bInfiniteWill)
            {
                if (t)
                {
                    CTCHeroStats* stats = (CTCHeroStats*)t->GetTC(TCI_HERO_STATS);
                    stats->m_nStamina = 10000;
                }
            }

            if (GetMenu().m_bCustomCameraPos && GetMenu().ms_bFreeCam && GetMenu().m_nFreeCameraMode == FREE_CAMERA_CUSTOM)
                FreeCamera::Update();
        }
    }
}

void HookRegularUpdate()
{
    if (!InGame())
        return;

    CWorld* wrld = CMainGameComponent::Get()->GetWorld();

    if (wrld)
    {
        if (wrld->isLoadSave()) {
            FableMenu::ms_bDisableCreateParticle = true;
            FableMenu::m_vCreatedParticles.clear();
            FableMenu::m_vAttachedParticles.clear();
        }
        else if (FableMenu::ms_bDisableCreateParticle) {
            FableMenu::ms_bDisableCreateParticle = false;
        }
    }
}

void FableMenu::TakeActionItem(CThing* creature, char* objectName)
{
    int id = CThing::GetThingID(objectName);
    CThing* thing = CreateThing(id, creature->GetPosition(), 0, 0, 0, (char*)"thing");
    CTCBase* base = (CTCBase*)GameMalloc(100);
    CTCBase* action = (CTCBase*)GameMalloc(180);

    creature->ClearQueuedActions();
    creature->FinishCurrentAction();

    if (objectName == "OBJECT_VILLAGE_TAVERN_JUG")
        new CCreatureAction_PickUpJugToFill((CCreatureAction_PickUpJugToFill*)action, creature, thing);
    else if (objectName == "OBJECT_CRATE_SMALL_EXPLOSIVE_01_USABLE")
        new CCreatureAction_PickUpGenericBox((CCreatureAction_PickUpGenericBox*)action, creature, thing);
    creature->SetCurrentAction(action);
}

bool InGame()
{
    if (CMainGameComponent::Get())
    {
        if (CMainGameComponent::Get()->GetWorld())
            return true;
    }
    return false;
}

bool IsWindowFocused()
{
    return GetMenu().m_bIsFocused;
}

char* GetUTF8String(wchar_t* name)
{
    char utf8Buff[256] = {};
    int size = WideCharToMultiByte(CP_UTF8, 0, name, -1, NULL, 0, NULL, NULL);

    WideCharToMultiByte(CP_UTF8, 0, name, -1, utf8Buff, size, NULL, NULL);

    return utf8Buff;
}

void CopyToClipboard(char* name, char* description)
{
    const size_t len = strlen(name) + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), name, len);
    GlobalUnlock(hMem);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
    Notifications->SetNotificationTime(2500);
    Notifications->PushNotification(GetLang().ReadString("Lists", "list_notification", "Copied %s to clipboard!"), description);
}

void CopyToClipboard(char* name)
{
    const size_t len = strlen(name) + 1;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), name, len);
    GlobalUnlock(hMem);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
    Notifications->SetNotificationTime(2500);
    Notifications->PushNotification(GetLang().ReadString("Lists", "list_notification", "Copied %s to clipboard!"), name);
}

std::string GetClipboardText() {
    std::string result;

    if (!OpenClipboard(nullptr)) {
        return result;
    }

    if (IsClipboardFormatAvailable(CF_TEXT)) {
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData != nullptr) {
            char* pszText = static_cast<char*>(GlobalLock(hData));
            if (pszText != nullptr) {
                result = pszText;
                GlobalUnlock(hData);
            }
        }
    }

    // Закрываем буфер обмена
    CloseClipboard();

    return result;
}

bool ButtonAutoSize(const char* text) {
    return ButtonAutoSize(text, 20.0f, 0.0f);
}

bool ButtonAutoSize(const char* text, float extraWidth, float extraHeight) {
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImVec2 buttonSize = ImVec2(
        textSize.x + ImGui::GetStyle().FramePadding.x * 2 + extraWidth,
        textSize.y + ImGui::GetStyle().FramePadding.y * 2 + extraHeight
    );

    return ImGui::Button(text, buttonSize);
}