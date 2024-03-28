#include "imgui/imgui.h"
#include "IniReader.h"
#include <iostream>
#include <Windows.h>
#include "FableMenu.h"
#include "Fable.h"
#include "eDirectX9Hook.h"
#include "utils/MemoryMgr.h"
#include "helper/eKeyboardMan.h"
#include "eSettingsManager.h"
#include "helper/eMouse.h"
#include "plugin/FreeCamera.h"
#include "eNotifManager.h"
#include <string>
#include "CsvReader.h"
#include <map>

using namespace Memory::VP;
FableMenu* TheMenu = new FableMenu();

bool FableMenu::ms_bFreeCam = false;
bool FableMenu::m_bCustomCameraPos = false;
bool FableMenu::ms_bDisableHUD = false;
bool FableMenu::m_bCustomCameraFOV = false;
bool FableMenu::ms_bChangeTime = false;
float FableMenu::m_fTime = 0.0f;

CIniReader lang("fablemenu_lang.ini");

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

const char* szFactions[] = {
	"FACTION_HERO",
	"FACTION_VILLAGERS",
	"FACTION_BANDITS",
	"FACTION_NEUTRALS",
	"FACTION_TWINBLADE_CAMP_BANDITS",
	"FACTION_MONSTERS",
	"FACTION_GUILD_APPRENTICES_GOOD",
	"FACTION_GUILD_SERVANTS",
	"FACTION_BANDITS_FRIENDLY",
	"FACTION_VILLAGER",
	"FACTION_GUARDS_ENEMY",
	"FACTION_BANDIT",
	"FACTION_BODYGUARD",
	"FACTION_SCRIPT_NEUTRAL",
	"FACTION_NEUTRAL",
	"FACTION_MAP_KEEPER",
	"FACTION_PRISONER",
	"FACTION_PRISON_GUARD",
	"FACTION_TRADERS",
	"FACTION_PICNIC_AREA",
	"FACTION_GUILD",
	"FACTION_MONSTER",
};


const char* szBuiltInQuests[] = {
    "CS_OakValeRevisited",
    "CS_PlayCutscene",
    "ChapterAndSceneManager",
    "CreatureGenerators",
    "DummyQuestForHeroLevels",
    "DummyQuestForScarletRoseStatue",
    "ForceKHGGatesOpen",
    "Gameflow",
    "GameflowAssistance",
    "HeroBoasts",
    "Hook_BalverinesInKHGPreWhiteBalv",
    "Hook_BoastingCrowd",
    "Hook_BowerstoneTeleportTutorial",
    "Hook_Fresco_01_EvilEpilogue",
    "Hook_Fresco_02_GoodEpilogue",
    "Hook_Fresco_03_JacksObsession",
    "Hook_Fresco_04_Kraken",
    "Hook_Fresco_05_MothersStory",
    "Hook_Fresco_06_Prison",
    "Hook_Fresco_07_OakValeRaid",
    "Hook_Fresco_08_SistersStory",
    "Hook_Fresco_09_TimePassing",
    "Hook_Fresco_10_UneasyAlliance",
    "Hook_Fresco_11_Wedding",
    "Hook_Fresco_12_KilledDragon",
    "Hook_Fresco_13_KilledScorpion",
    "LadyGreyWifeManager",
    "MarkStealthTest",
    "NPCDeath",
    "OracleMinigameTest",
    "PersonalScriptMain",
    "PersonalScript_GlobalThings",
    "QR_EscortTrader",
    "QR_EscortTrader_Manager",
    "QS_GuardianSisterInfo",
    "QS_GuardianSisterInfo2_SisterInBanditCamp",
    "QS_GuardianTrophyDealerInfo",
    "QS_MeetSister",
    "QS_ScytheInfo",
    "Q_AmbushTraders",
    "Q_AmbushTraders_ActivatePhase1",
    "Q_AmbushTraders_ActivatePhase2",
    "Q_AmbushTraders_ActivatePhase3",
    "Q_Arena",
    "Q_ArenaHoldingScript",
    "Q_AwakeningTheOracle",
    "Q_AwakeningTheOracleMinigame",
    "Q_BanditCamp",
    "Q_BanditCampBossBattle",
    "Q_BanditCampHoldingScript",
    "Q_BanditCamp_Barriers",
    "Q_BountyHunt",
    "Q_BowerstoneTownLifeIntro",
    "Q_BreakSiege",
    "Q_BreakSiege_Blockage",
    "Q_CinemaTest",
    "Q_DragonBossFight",
    "Q_EndGame",
    "Q_EndGameBossBattle",
    "Q_EndGameBossBattleScenery",
    "Q_EndGameFocalSites",
    "Q_EndGame_Barriers",
    "Q_FireHeart",
    "Q_GuildTraining",
    "Q_GuildTrainingDeparture",
    "Q_GuildTrainingMelee",
    "Q_GuildTrainingPreMelee",
    "Q_GuildTrainingSkill",
    "Q_GuildTrainingWill",
    "Q_GuildTrainingWoodsDeparture"
    "Q_GuildTrainingWoodsDeparture",
    "Q_GuildTrainingWoodsMelee",
    "Q_GuildTrainingWoodsWill",
    "Q_HangingTreeEvil",
    "Q_HangingTreeGood",
    "Q_HeroSouls",
    "Q_HeroSoulsArena",
    "Q_HeroSoulsBriar",
    "Q_HeroSoulsBriarMagicBarrier",
    "Q_HeroSoulsGuildmaster",
    "Q_HeroSoulsMother",
    "Q_HeroSoulsNostro",
    "Q_HeroSoulsPermanentArena",
    "Q_HeroSoulsThunder",
    "Q_HerosOldHouse",
    "Q_HobbeCave",
    "Q_HobbeCaveBarrier",
    "Q_HobbeCaveGranny",
    "Q_HobbeToothContest",
    "Q_MinionCamp",
    "Q_MinionClifftopChase",
    "Q_MinionClifftopChase_Barriers",
    "Q_NewOakValeIntro",
    "Q_NewOakValeIntro_PreAttack",
    "Q_OakValeBanditRaid",
    "Q_OpeningGraveyardSecretPassage",
    "Q_OrchardFarmRaid",
    "Q_OrchardFarmRaidEvil",
    "Q_OrchardFarmRaidGood",
    "Q_OrchardFarm_Barricade",
    "Q_PrisonEscapeFromCell",
    "Q_PrisonEscapeRescueMother",
    "Q_PrisonRace",
    "Q_PrisonWardenGame",
    "Q_RansomVictim",
    "Q_RansomVictimChiefsHouse",
    "Q_RansomVictimNaturesRevenge",
    "Q_RansomVictimRevenge",
    "Q_RansomVictimStarted",
    "Q_RansomVictimStartedNature",
    "Q_SecretPassage",
    "Q_SummoningTheShip",
    "Q_SunnyvaleMaster",
    "Q_TentacleKrakenBossFight",
    "Q_TraderConflictEvil",
    "Q_TraderConflictGood",
    "Q_TraderConflictGood_Extras",
    "Q_TraderEscort",
    "Q_TraderEscortForceField",
    "Q_TraderPath",
    "Q_UndeadRising",
    "Q_WaspBoss",
    "Q_WhiteBalverineKnotholeGlade"
    "Q_WhiteBalverineKnotholeGlade",
    "Q_WhiteBalverineWW",
    "Q_WizardBattle",
    "Q__OakValeIntro_PostAttack",
    "ShowTargetedThingHealth",
    "TonyTestScripts",
    "V_AmbushScam",
    "V_AmbushScamEvil",
    "V_AmbushScamGood",
    "V_ArcheryCompetition",
    "V_ArcheryCompetition_Activate",
    "V_AssassinAttacks",
    "V_AssassinAttacks_Activate",
    "V_BanditCampPath",
    "V_BanditToll",
    "V_BeardyBaldy",
    "V_BeggarAndChild",
    "V_BodyGuard",
    "V_BookCollecting",
    "V_Bordello",
    "V_ChapelOfEvil",
    "V_ChapelOfEvil_Activate",
    "V_ChickenKicking",
    "V_DemonDoors",
    "V_ExposeMayor",
    "V_ExposeMayorRhodri",
    "V_Fisherman",
    "V_FishingCompetition",
    "V_FisticuffsClub",
    "V_FisticuffsClubOnlyAtNight",
    "V_GhostGrannyNecklace",
    "V_GuildMaster",
    "V_HauntedHouse",
    "V_HelpTips",
    "V_HeroDolls",
    "V_HeroDuel",
    "V_HiddenBooty",
    "V_HiddenBooty_Activate",
    "V_IntroductionToTrophies",
    "V_KnotholeGladeGates",
    "V_LostTrader",
    "V_MayorsInvitation",
    "V_MazeResearch",
    "V_MurderTwistFinal",
    "V_Oracle",
    "V_ParanoidWhispers",
    "V_PicnicAreaAfterWaspBoss",
    "V_RandomPopulationSim",
    "V_RockTrollFirstEncounter",
    "V_RockTrollFirstEncounter_Activate",
    "V_SickChild",
    "V_SickChildBarrowFields",
    "V_SickChild_Activate",
    "V_SingingStones",
    "V_SingingStones_Activate",
    "V_StatueMaster",
    "V_SwordInTheStone",
    "V_TalentlessBard",
    "V_TempleOfLight",
    "V_TempleOfLight_Activate",
    "V_TourGuide",
    "V_TourGuide_Activate",
    "V_TravellingBeggar",
    "V_TravellingBully",
    "V_TravellingHeroes",
    "V_TrophyDealer",
};


struct Translation {
	std::string original;
	std::string translate;
	std::string note;
	std::string crash;
	std::string limited;
};

// Создаем экземпляр класса для чтения CSV-файла
CCsvReader objReader("obj.csv", true);
std::map<std::string, Translation> object_dir;
CCsvReader crtReader("crt.csv", true);
std::map<std::string, Translation> crt_dir;

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
bool isPairEmpty(const std::pair<K, V>& pair) {
	return std::is_default_constructible<K>::value && pair.first == K();
}

void init_obj_list() {
	try {

		if (!objReader.OpenCsv()) {
			throw u8"obj.csv does not open";
		}

		std::vector<std::string> columns;
		while (!(columns = objReader.ReadLine()).empty()) {
			object_dir[columns[0]] = { columns[1], columns[2], columns[3], columns[4], "0" };
		}

	}
	catch (char* error) {
		Notifications->SetNotificationTime(5000);
		Notifications->PushNotification(error);
	}
	catch (const std::exception& ex) {
		Notifications->SetNotificationTime(5000);
		Notifications->PushNotification(ex.what());
	}
}

void init_crt_list() {
	try {

		if (!crtReader.OpenCsv()) {
			throw u8"crt.csv does not open";
		}

		std::vector<std::string> columns;
		while (!(columns = crtReader.ReadLine()).empty()) {
			crt_dir[columns[0]] = { columns[1], columns[2], columns[3], columns[4], columns[5] };
		}

	}
	catch (char* error) {
		Notifications->SetNotificationTime(5000);
		Notifications->PushNotification(error);
	}
	catch (const std::exception& ex) {
		Notifications->SetNotificationTime(5000);
		Notifications->PushNotification(ex.what());
	}
}

void FableMenu::Init()
{
	init_obj_list();
	init_crt_list();

	sprintf(szFactionName, szFactions[0]);
}

void FableMenu::Draw()
{
	if (!m_bIsActive)
		return;
	ImGui::GetIO().MouseDrawCursor = true;

	ImGui::Begin(lang.ReadString("Window", "main_window", "FableMenu by ermaccer"), &m_bIsActive, ImGuiWindowFlags_MenuBar);
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu(lang.ReadString("Menu", "menu_settings", "Settings")))
			{
				m_bSubmenuActive[SM_Settings] = true;
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu(lang.ReadString("Menu", "menu_help", "Help")))
			{
                if (ImGui::MenuItem(lang.ReadString("Menu", "menu_help_creatures", "Creature List")))
                {
                    m_bSubmenuActive[SM_Creature_List] = true;
                }
                if (ImGui::MenuItem(lang.ReadString("Menu", "menu_help_objects", "Objects List")))
                {
                    m_bSubmenuActive[SM_Object_List] = true;
                }
                if (ImGui::BeginMenu(lang.ReadString("Menu", "menu_about", "About")))
                {
                    ImGui::MenuItem(lang.ReadString("Menu", "menu_about_version" FABLEMENU_VERSION, "Version: " FABLEMENU_VERSION));
					ImGui::MenuItem((lang.ReadString("Menu", "menu_about_date" __DATE__, "Date: " __DATE__)));
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
				if (ImGui::BeginTabItem(lang.ReadString("Tabs", "tab_player", "Player")))
				{
					DrawPlayerTab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem(lang.ReadString("Tabs", "tab_camera", "Camera")))
				{
					DrawCameraTab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem(lang.ReadString("Tabs", "tab_world", "World")))
				{
					DrawWorldTab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem(lang.ReadString("Tabs", "tab_quest", "Quest")))
				{
					DrawQuestTab();
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem(lang.ReadString("Tabs", "tab_misc", "Misc.")))
				{
					DrawMiscTab();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		else
			ImGui::TextWrapped(lang.ReadString("Preload", "not_ready", "Not ready!"));
	}
	ImGui::End();

	if (m_bSubmenuActive[SM_Settings])
		DrawSettings();

	if (m_bSubmenuActive[SM_Creature_List])
		DrawCreatureList();

	if (m_bSubmenuActive[SM_Object_List]) {
		DrawObjectList();
	}
		
        
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
}

void FableMenu::DrawPlayerTab()
{
	CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetPlayer();
	if (plr)
	{
		CThing* t = plr->GetCharacterThing();
		CTCHeroStats* stats = t->GetHeroStats();
		CTCHeroMorph* morph = t->GetHeroMorph();
		CTCHeroExperience* exp = t->GetHeroExperience();
		CTCLook* look = t->GetLook();
		CTCHero* hero = t->GetHero();
		if (stats)
		{
			if (ImGui::CollapsingHeader(lang.ReadString("Data", "data_text", "Data")))
			{
				ImGui::InputFloat(lang.ReadString("Data", "data_health", "Health"), &t->m_fHealth);
				ImGui::InputFloat(lang.ReadString("Data", "data_max_health", "Max. Health"), &t->m_fMaxHealth);
				ImGui::InputInt(lang.ReadString("Data", "data_max_will", "Will"), &stats->m_nWillPower, 0);
				ImGui::InputInt(lang.ReadString("Data", "data_max_will", "Max. Will"), &stats->m_nMaxWillPower, 0);
				ImGui::InputInt(lang.ReadString("Data", "data_gold", "Gold"), &stats->m_nGold);
				ImGui::InputFloat(lang.ReadString("Data", "data_age", "Age"), &stats->m_fAge);
				if (hero)
				{
					ImGui::Checkbox(lang.ReadString("Data", "data_can_use_weapons", "Can Use Weapons"), &hero->m_bCanUseWeapons);
					ImGui::Checkbox(lang.ReadString("Data", "data_can_use_will", "Can Use Will"), &hero->m_bCanUseWill);
				}
				if (exp)
				{
					ImGui::Separator();
					ImGui::Text(lang.ReadString("Exp", "exp_text", "Experience"));
					ImGui::Separator();
					ImGui::InputInt(lang.ReadString("Exp", "exp_general", "General"), &exp->m_nGeneralExperience, 0);
					ImGui::InputInt(lang.ReadString("Exp", "exp_strength", "Strength"), &exp->m_pExperience[EXPERIENCE_STRENGTH], 0);
					ImGui::InputInt(lang.ReadString("Exp", "exp_will", "Will"), &exp->m_pExperience[EXPERIENCE_WILL], 0);
					ImGui::InputInt(lang.ReadString("Exp", "exp_skill", "Skill"), &exp->m_pExperience[EXPERIENCE_SKILL], 0);
				}
				enum eMovementTypes {
					ST_SLOW_WALK,
					ST_WALK,
					ST_JOG,
					ST_RUN,
					ST_SPRINT,
					STANDARD_FLY,
					TOTAL_MOVEMENT_TYPES
				};
				ImGui::Separator();
				ImGui::Text(lang.ReadString("Movement", "mov_text", "Movement Speed"));
				ImGui::Separator();

				static const char* movementNames[] = {
					lang.ReadString("Movement", "mov_slow", "Slow Walk"),
					lang.ReadString("Movement", "mov_walk", "Walk"),
					lang.ReadString("Movement", "mov_jog", "Jog"),
					lang.ReadString("Movement", "mov_run", "Run"),
					lang.ReadString("Movement", "mov_roll", "Roll")
				};

				for (int i = 0; i < TOTAL_MOVEMENT_TYPES - 1; i++)
				{
					float& speed = *(float*)((int)t + 0x18C + (i * sizeof(int)));

					ImGui::InputFloat(movementNames[i], &speed);
				}

			}
		}
		if (morph)
		{
			if (ImGui::CollapsingHeader(lang.ReadString("Morph", "morph_text", "Morph")))
			{
				ImGui::SliderFloat(lang.ReadString("Morph", "morph_strength", "Strength"), &morph->m_fStrength, 0.00f, 1.0f);
				ImGui::SliderFloat(lang.ReadString("Morph", "morph_berserk", "Berserk"), &morph->m_fBerserk, 0.00f, 1.0f);
				ImGui::SliderFloat(lang.ReadString("Morph", "morph_will", "Will"), &morph->m_fWill, 0.00f, 1.0f);
				ImGui::SliderFloat(lang.ReadString("Morph", "morph_skill", "Skill"), &morph->m_fSkill, 0.00f, 1.0f);
				ImGui::SliderFloat(lang.ReadString("Morph", "morph_age", "Age"), &morph->m_fAge, 0.00f, 1.0f);
				ImGui::SliderFloat(lang.ReadString("Morph", "morph_alignment", "Alignment"), &morph->m_fAlign, 0.00f, 1.0f);
				ImGui::SliderFloat(lang.ReadString("Morph", "morph_fatness", "Fatness"), &morph->m_fFat, 0.00f, 1.0f);

				ImGui::Checkbox(lang.ReadString("Morph", "morph_kid", "Kid"), &morph->m_bKid);
				if (ImGui::Button(lang.ReadString("Morph", "morph_update", "Update"), ImVec2(-FLT_MIN, 0)))
					morph->m_bUpdate = true;
			}
		}
		if (ImGui::CollapsingHeader(lang.ReadString("Spell", "spell_text", "Spell Data")))
		{
			int& curSummonCreature = *(int*)(0x138306C);
			ImGui::TextWrapped(lang.ReadString("Spell", "spell_current", "Current Summon Creature ID"));
			ImGui::SameLine();
			ShowHelpMarker(lang.ReadString("Spell", "spell_help", "You can get desired creature ID from World->Cretures section."));
			ImGui::PushItemWidth(-FLT_MIN);
			ImGui::InputInt("##creaturesumid", &curSummonCreature);
			ImGui::PopItemWidth();
			ImGui::Separator();

		}
		if (ImGui::CollapsingHeader(lang.ReadString("Input", "input_text", "Input")))
		{
			ImGui::TextWrapped(lang.ReadString("Input", "input_help", ""));
			if (ImGui::Button(lang.ReadString("Input", "input_disable", "Disable"), { -FLT_MIN, 0 }))
			{
				if (plr)
					plr->SetMode(17, false);
			}
			if (ImGui::Button(lang.ReadString("Input", "input_enable", "Enable"), { -FLT_MIN, 0 }))
			{
				if (plr)
					plr->ClearMode(17);
			}
		}
		if (ImGui::CollapsingHeader(lang.ReadString("Appearance", "ap_text", "Appearance")))
		{
			static int alpha = 255;
			ImGui::SliderInt(lang.ReadString("Appearance", "ap_alpha", "Alpha"), &alpha, 0, 255);
			
			static ImVec4 color = {1.0, 1.0, 1.0, 1.0};
			ImGui::ColorEdit3(lang.ReadString("Appearance", "ap_color", "Color"), (float*)&color);

			union rgba {
				char rgba[4];
				int color;
			};

			rgba rcolor;

			rcolor.rgba[0] = (unsigned char)((color.z) * 255.0f);
			rcolor.rgba[1] = (unsigned char)((color.y) * 255.0f);
			rcolor.rgba[2] = (unsigned char)((color.x) * 255.0f);
			rcolor.rgba[3] = (unsigned char)((color.w) * 255.0f);


			static float scale = 1.0f;
			ImGui::InputFloat(lang.ReadString("Appearance", "ap_scale", "Scale"), &scale);

			if (ImGui::Button(lang.ReadString("Appearance", "ap_update", "Update"), {-FLT_MIN, 0}))
			{
				CTCGraphicAppearance* ga = t->GetGraphicAppearance();
				if (ga)
				{
					ga->SetAlpha(alpha);
					ga->SetColor(&rcolor.color, t->GetHero());
					ga->SetScale(scale);
				}

			}

		}
		ImGui::Separator();
		ImGui::Text(lang.ReadString("Player", "player_position", "Position"));
		ImGui::InputFloat3("", &t->GetPosition()->X);
}
}

void FableMenu::DrawCameraTab()
{
	if (TheCamera)
	{
		ImGui::Checkbox(lang.ReadString("Camera", "camera_position", "Set Camera Position"), &m_bCustomCameraPos);
		ImGui::InputFloat3("X | Y | Z", &camPos.X);

		ImGui::Checkbox(lang.ReadString("Camera", "camera_fov_checkbox", "Set FOV"), &m_bCustomCameraFOV);

		if (m_bCustomCameraFOV)
			ImGui::InputFloat(lang.ReadString("Camera", "camera_fov", "FOV"), &TheCamera->FOV);
		ImGui::Separator();
	}

	ImGui::Checkbox(lang.ReadString("Camera", "camera_free_checkbox", "Free Camera"), &ms_bFreeCam);
	if (ms_bFreeCam)
	{
		ImGui::Separator();
		if (!m_bCustomCameraPos)
			ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), lang.ReadString("Camera", "camera_check_set_position", "Check \"Set Camera Position\""));

		ImGui::Text(lang.ReadString("Camera", "camera_free_type", "Free Camera Type"));
		ImGui::Separator();
		ImGui::RadioButton(lang.ReadString("Camera", "camera_custom", "Custom (Recommended)"), &m_nFreeCameraMode, FREE_CAMERA_CUSTOM);
		ImGui::SameLine();
		ShowHelpMarker(lang.ReadString("Camera", "camera_free_custom_help", "A custom free camera implementation, uses NUMPAD keys by default to move the camera. Mouse and key settings can be changed in the Settings menu.")); 
		ImGui::RadioButton(lang.ReadString("Camera", "camera_original", "Original"), &m_nFreeCameraMode, FREE_CAMERA_ORIGINAL);
		if (m_nFreeCameraMode == FREE_CAMERA_CUSTOM)
		{
			ImGui::Separator();
			ImGui::InputFloat(lang.ReadString("Camera", "camera_free_speed", "Free Camera Speed"), &m_fFreeCamSpeed);
		}

		ImGui::Separator();
	}

	if (ImGui::Button(lang.ReadString("Camera", "camera_teleport_player_to_camera", "Teleport Player To Camera Location"), {-FLT_MIN, 0 }))
	{
		CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetPlayer();
		if (plr)
		{
			CThing* t = plr->GetCharacterThing();
			*t->GetPosition() = TheCamera->pos;
		}
	}

}

void FableMenu::DrawWorldTab()
{
	CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetPlayer();
	CWorld* wrld = CMainGameComponent::Get()->GetWorld();
	if (wrld)
	{
		ImGui::Text(lang.ReadString("World", "world_text", "Settings"));
		ImGui::Separator();
		ImGui::Checkbox(lang.ReadString("World", "world_minimap", "Minimap"), &wrld->m_bMinimap);
		if (plr)
		{
			bool& enemies = *(bool*)((int)plr + 0x21B);
			ImGui::Checkbox(lang.ReadString("World", "world_kill_mode", "Kill Mode"), &enemies);
		}
		bool& quest_regions = *(bool*)(0x1375741);
		ImGui::Checkbox(lang.ReadString("World", "world_quest_regions", "Quest Regions"), &quest_regions);
		ImGui::Separator();
	}
	if (wrld)
	{
		if (ImGui::CollapsingHeader(lang.ReadString("World", "world_time_text", "Time")))
		{
			int time = *(int*)((int)wrld + 28);
			if (time)
			{
				float& timeStep = *(float*)((int)time + 16);
				ImGui::SliderFloat(lang.ReadString("World", "world_time_step", "Time Step"), &timeStep, 0.001f, 1.0f);

				ImGui::Checkbox(lang.ReadString("World", "world_set_time", "Set Time"), &ms_bChangeTime);
				if (ms_bChangeTime)
				{
					float& curTime = *(float*)((int)time + 8);
					ImGui::SliderFloat(lang.ReadString("World", "world_time", "Time"), &curTime, 0.0, 1.0f);
				}

			}
		}
	}

	if (ImGui::CollapsingHeader(lang.ReadString("Creating", "cr_text", "Spawning")))
	{
        ImGui::TextWrapped(lang.ReadString("Creating", "cr_id_help", "ID is available in object/creatures list in Help menu."));
        ImGui::Separator();
		static CVector pos = {};
		static char obj_text[512] = ""; 
		static int id = 0;
		static int caiID = 0;
		static int objCount = 1;
		static std::pair<std::string, Translation> selected_;
		static char text[512] = { };
		static bool crt = false; // true -> creature, false -> object
		static std::string crash = "0";
		static std::string limited = "0";
		static bool playerFollower = false;
		ImGui::TextWrapped(lang.ReadString("Creating", "cr_position", "Spawn Position (X | Y | Z)"));
		ImGui::PushItemWidth(-FLT_MIN);
		ImGui::InputFloat3("", &pos.X);
		ImGui::PopItemWidth();
		if (ImGui::Button(lang.ReadString("Creating", "cr_get_position", "Get Player Position"), { -FLT_MIN, 0 }))
		{
			if (plr)
				pos = *plr->GetCharacterThing()->GetPosition();
		}

		ImGui::Text(lang.ReadString("Creating", "cr_id_name", "Enter ID or Original Name"));
		ImGui::PushItemWidth(-FLT_MIN);
		if (ImGui::InputText("##id", obj_text, sizeof(obj_text))) {
			std::string ObjString(obj_text);
			auto result_obj = findValue(object_dir, ObjString);
			auto result_crt = findValue(crt_dir, ObjString);

			if (!isPairEmpty(result_obj)) {
				id = atoi(result_obj.first.c_str());
				crt = false;
				selected_ = result_obj;
			}
			else if (!isPairEmpty(result_crt)) {
				id = atoi(result_crt.first.c_str());
				crt = true;
				selected_ = result_crt;
			}
			else {
				id = 0;
			}
		}
		ImGui::PopItemWidth();

		if (id > 0) {
			ImGui::TextWrapped(lang.ReadString("Creating", "cr_original", "Original name: %s"), selected_.second.original.c_str());
			ImGui::TextWrapped(lang.ReadString("Creating", "cr_translate", "Translate: %s"), selected_.second.translate.c_str());
			ImGui::TextWrapped(lang.ReadString("Creating", "cr_note", "Note: %s"), selected_.second.note.c_str());

			crash = selected_.second.crash.c_str();
			limited = selected_.second.limited.c_str();
		}

		if (crt && limited != "1") {
			ImGui::Separator();
			ImGui::InputInt(lang.ReadString("Creating", "cr_owner_id", "Owner ID"), &caiID);

			ImGui::TextWrapped(lang.ReadString("Creating", "cr_faction", "Faction"));
			ImGui::PushItemWidth(-FLT_MIN);
			if (ImGui::BeginCombo("##faclist", szFactionName))
			{
				for (int n = 0; n < IM_ARRAYSIZE(szFactions); n++)
				{
					bool is_selected = (szFactionName == szFactions[n]);
					if (ImGui::Selectable(szFactions[n], is_selected))
						sprintf(szFactionName, szFactions[n]);
					if (is_selected)
						ImGui::SetItemDefaultFocus();

				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();
			ImGui::Checkbox(lang.ReadString("Creating", "cr_as_follower", "Create as player follower"), &playerFollower);
			ImGui::SameLine();
			ShowHelpMarker(lang.ReadString("Creating", "cr_as_follower_help", "Creature will follow and defend player, this only works with some creatures (usually those that are simple enough, eg. swords or spirits)"));

			ImGui::Separator();
		} 
		else {
		}

		if (id <= 0)
			ImGui::TextWrapped(lang.ReadString("Creating", "cr_wrong", "Wrong ID or NAME!"));
		else if (crash == "1") {
			ImGui::TextWrapped(lang.ReadString("Creating", "cr_crash", "This will cause a crash!"));
		}
		else {
			ImGui::InputInt(lang.ReadString("Creating", "cr_count", "Select the count to create"), &objCount);
			if (ImGui::Button(lang.ReadString("Creating", "cr_create", "Create"))) {
				for (int caiNumber = 0; caiNumber < objCount; caiNumber++) {
					if (crt) {
						CThing* creature = CreateCreature(id, &pos, caiID);
						if (creature && limited != "1")
						{
							if (plr)
							{
								CTCEnemy* enemy = creature->GetEnemy();
								CCharString faction(szFactionName);
								enemy->SetFaction(&faction);

								if (playerFollower)
								{
									CTCRegionFollower* rf = plr->GetCharacterThing()->GetRegionFollower();
									enemy->AddAlly(plr->GetCharacterThing());
									CIntelligentPointer ptr(creature);

									rf->AddFollower(*ptr);
								}
								else
								{
									if (strcmp(szFactionName, "FACTION_HERO") == 0)
										enemy->AddAlly(plr->GetCharacterThing());
								}
							}

						}
					}
					else {
						CreateThing(id, &pos, 0, 0, 0, "newObj");
					}
				}
			}
		}
	}
}

void FableMenu::DrawQuestTab()
{
	if (!CQuestManager::Get())
	  return;


	CQuestManager* q = CQuestManager::Get();

    ImGui::TextWrapped(lang.ReadString("Quest", "quest_note", "NOTE: Quest changes might break your savegame! To be safe, do any quest changes on a backup/alternative save."));

	static char scriptName[256] = {};
	if (ImGui::CollapsingHeader(lang.ReadString("Quest", "quest_status_control", "Status Control")))
	{
		ImGui::Text(lang.ReadString("Quest", "quest_name", "Quest Name"));
        static bool writeName = false;

		ImGui::PushItemWidth(-FLT_MIN);

        if (!writeName)
        {
            if (ImGui::BeginCombo("##faclist", scriptName))
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
        ImGui::Checkbox(lang.ReadString("Quest", "quest_manual_input", "Manual Input"), &writeName);

		ImGui::PopItemWidth();

        if (strlen(scriptName) > 0)
        {
            if (ImGui::Button(lang.ReadString("Quest", "quest_activate", "Activate"), { -FLT_MIN, 0 }))
            {
                CCharString str(scriptName);
                if (q->IsActive(&str))
                {
                    Notifications->SetNotificationTime(5000);
                    Notifications->PushNotification(lang.ReadString("Quest", "quest_already_active", "Quest \"%s\is already active!"), scriptName);
                }
                else
                {
                    if (q->ActivateQuest(&str, false, true))
                    {
                        Notifications->SetNotificationTime(5000);
                        Notifications->PushNotification(lang.ReadString("Quest", "quest_activated", "Quest \"%s\activated!"), scriptName);
                    }
                    else
                    {
                        Notifications->SetNotificationTime(5000);
                        Notifications->PushNotification(lang.ReadString("Quest", "quest_failed", "Failed to activate quest \"%s\"!"), scriptName);
                    }
                }

            }
            if (ImGui::Button(lang.ReadString("Quest", "quest_deactivate", "Deactivate"), { -FLT_MIN, 0 }))
            {
                CCharString str(scriptName);
                if (!q->IsActive(&str))
                {
                    Notifications->SetNotificationTime(5000);
                    Notifications->PushNotification(lang.ReadString("Quest", "quest_not_active", "Quest \"%s\is not active!"), scriptName);
                }
                else
                {
                    q->DeactivateQuest(&str, 0);
                    Notifications->SetNotificationTime(5000);
                    Notifications->PushNotification(lang.ReadString("Quest", "quest_deactivated", "Quest \"%s\deactivated!"), scriptName); 
                }
            }
        }
		
	}


	if (ImGui::CollapsingHeader(lang.ReadString("Tweaks", "tweaks_text", "Tweaks")))
	{
		ImGui::Checkbox(lang.ReadString("Tweaks", "tweaks_bodyguard_limit", "No BodyGuards Limit"), &m_bNoBodyGuardsLimit);
		ImGui::SameLine(); ShowHelpMarker(lang.ReadString("Tweaks", "tweaks_bodyguard_limit_help", "Allows to hire all bodyguards. Default limit is 2."));
	}
}

void FableMenu::DrawMiscTab()
{
	ImGui::Checkbox(lang.ReadString("Misc", "misc_hud", "Display HUD"), &GetHud()->m_bDisplay);


	CWorld* wrld = CMainGameComponent::Get()->GetWorld();
	if (wrld)
	{
		CBulletTimeManager* time = wrld->GetBulletTime();
		if (time)
		{
			ImGui::Checkbox(lang.ReadString("Misc", "misc_slowmotion", "Slowmotion"), &time->m_bActive);

		}
	}

	ImGui::Separator();
	ImGui::Text(lang.ReadString("Misc", "misc_cheats", "Cheats"));
	ImGui::Separator();
	ImGui::Checkbox(lang.ReadString("Misc", "misc_inf_health", "Infinite Health"), &m_bGodMode);
	ImGui::Checkbox(lang.ReadString("Misc", "misc_inf_will", "Infinite Will"), &m_bInfiniteWill);
	ImGui::Separator();
#ifdef _DEBUG
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

	CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetPlayer();
	if (plr)
	{
		CThing* t = plr->GetCharacterThing();
		ImGui::Text("Player Stats: 0x%X", t->GetHeroStats());
		ImGui::Text("Player Morph: 0x%X\n", t->GetHeroMorph());
		ImGui::Text("Player Experience: 0x%X\n", t->GetHeroExperience());
		ImGui::Text("Player Thing: 0x%X\n", t);
		ImGui::Text("Player Physics: 0x%X\n", t->GetPhysics());
		ImGui::Text("Player: 0x%X\n", plr);
		ImGui::Text("Draw: 0x%X\n", t->GetGraphicAppearance());
		ImGui::Text("Script Manager: 0x%X\n", wrld->GetScriptInfoManager());
	}
#endif
}

void FableMenu::DrawSettings()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.5f, 0.5f });
	ImGui::SetNextWindowPos({ ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f }, ImGuiCond_Once, { 0.5f, 0.5f });
	ImGui::SetNextWindowSize({ 700,700 }, ImGuiCond_Once);
	ImGui::Begin(lang.ReadString("Settings", "stn_text", "Settings"), &m_bSubmenuActive[SM_Settings]);

	static int settingID = 0;
	static const char* settingNames[] = {
		lang.ReadString("Settings", "stn_menu", "Menu"),
		lang.ReadString("Settings", "stn_ini", "INI"),
		lang.ReadString("Settings", "stn_keys", "Keys"),
		lang.ReadString("Settings", "stn_mouse", "Mouse")
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
		ImGui::TextWrapped(lang.ReadString("MenuMain", "stn_menu_text", "All user settings are saved to fablemenu_user.ini."));
		ImGui::Text(lang.ReadString("MenuMain", "stn_menu_scale", "Menu scale"));
		ImGui::PushItemWidth(-FLT_MIN);
		ImGui::InputFloat("", &SettingsMgr->fMenuScale);
		ImGui::PopItemWidth();
		break;
	case INI:
		ImGui::TextWrapped(lang.ReadString("INI", "stn_ini_text", "These settings control FableMenu.ini options. Any changes require game restart to take effect."));
		ImGui::LabelText("", lang.ReadString("INI", "stn_ini_core", "Core"));
		ImGui::Separator();
		ImGui::Checkbox(lang.ReadString("INI", "stn_ini_slowmotion", "Slowmotion Spell/Effect Affects Everything"), &SettingsMgr->bSlowMotionEffectsEverything); 
		ImGui::Checkbox(lang.ReadString("INI", "stn_ini_windowed", "Windowed Mode"), &SettingsMgr->bUseBuiltInWindowedMode);
		ImGui::Separator();

		break;
	case KEYS:
		if (m_bPressingKey)
			ImGui::TextColored(ImVec4(0.f, 1.f, 0.3f, 1.f), lang.ReadString("Keys", "keys_press_key", "Press a key!"));

		if (ImGui::Button(lang.ReadString("Keys", "keys_reset_btn", "Reset Keys"), { -FLT_MIN, 0 }))
		{
			SettingsMgr->ResetKeys();
			Notifications->SetNotificationTime(5000);
			Notifications->PushNotification(lang.ReadString("Keys", "keys_reset_notification", "Keys reset! Remember to save.")); 
		}

		ImGui::Separator();
		ImGui::LabelText("", lang.ReadString("Keys", "keys_core", "Core"));
		ImGui::Separator();
		KeyBind(&SettingsMgr->iMenuOpenKey, lang.ReadString("Keys", "keys_open_close", "Open/Close Menu"), "menu");
		ImGui::Separator();
		ImGui::LabelText("", lang.ReadString("Keys", "keys_camera", "Camera"));
		ImGui::Separator();

		KeyBind(&SettingsMgr->iFreeCameraKeyForward, "Forward", "x_plus");
		KeyBind(&SettingsMgr->iFreeCameraKeyBack, "Back", "x_minus");
		KeyBind(&SettingsMgr->iFreeCameraKeyLeft, "Left", "y_plus");
		KeyBind(&SettingsMgr->iFreeCameraKeyRight, "Right", "y_minus");
		KeyBind(&SettingsMgr->iFreeCameraKeyUp, "Up", "z_plus");
		KeyBind(&SettingsMgr->iFreeCameraKeyDown, "Down", "z_minus");
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
		ImGui::TextWrapped(lang.ReadString("Mouse", "mouse_text", "All user settings are saved to fablemenu_user.ini.")); 
		ImGui::Text(lang.ReadString("Mouse", "mouse_sensitivity", "Sensitivity"));
		ImGui::PushItemWidth(-FLT_MIN);
		ImGui::SliderFloat("", &SettingsMgr->mouse.sens, 0, 10.0f);
		ImGui::PopItemWidth();
		ImGui::Checkbox(lang.ReadString("Mouse", "mouse_invert_x", "Invert X"), &SettingsMgr->mouse.invert_x);
		ImGui::Checkbox(lang.ReadString("Mouse", "mouse_invert_y", "Invert Y"), &SettingsMgr->mouse.invert_y);
		break;
	default:
		break;
	}

	if (ImGui::Button(lang.ReadString("Settings", "stn_btn_save", "Save"), { -FLT_MIN, 0 }))
	{
		Notifications->SetNotificationTime(5000);
		Notifications->PushNotification(lang.ReadString("Settings", "stn_save_notification", "Settings saved to FableMenu.ini and fablemenu_user.ini!"));
		eDirectX9Hook::ms_bShouldReloadFonts = true;
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
	ImGui::Begin(lang.ReadString("Creatures", "crt_list_text", "Creatures List"), &m_bSubmenuActive[SM_Creature_List]);

	static ImGuiTextFilter filter;
	ImGui::TextWrapped(lang.ReadString("Creatures", "crt_list_text_copy", "Click on any entry to copy to clipboard."));
	ImGui::Text(lang.ReadString("Creatures", "crt_list_search", "Search"));
	ImGui::PushItemWidth(-FLT_MIN);
	filter.Draw("");
	ImGui::PopItemWidth();

	ImGui::BeginChild("##olist", { 0, -ImGui::GetFrameHeightWithSpacing() }, true);

	for (auto &pair : crt_dir)
	{
		const std::string key = pair.first;
		const std::string display_text = pair.second.translate + " [" + pair.second.original + "]";

		if (filter.PassFilter(display_text.c_str()))
		{
			bool is_selected = (selectID == key);
			if (ImGui::Selectable(display_text.c_str(), is_selected))
			{
				selectID = key;

				const size_t len = key.length() + 1;
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
				memcpy(GlobalLock(hMem), key.c_str(), len);
				GlobalUnlock(hMem);
				OpenClipboard(NULL);
				EmptyClipboard();
				SetClipboardData(CF_TEXT, hMem);
				CloseClipboard();
				Notifications->SetNotificationTime(5000);
				Notifications->PushNotification(lang.ReadString("Creatures", "crt_list_notification", "Copied %s to clipboard!"));
			}
		}
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();

	ImGui::End();
}

void FableMenu::DrawObjectList()
{
	std::string selectID = "";

	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.5f, 0.5f });
	ImGui::SetNextWindowPos({ ImGui::GetIO().DisplaySize.x / 2.0f, ImGui::GetIO().DisplaySize.y / 2.0f }, ImGuiCond_Once, { 0.5f, 0.5f });
	ImGui::SetNextWindowSize({ 700,700 }, ImGuiCond_Once);
	ImGui::Begin(lang.ReadString("Objects", "obj_list_text", "Object List"), &m_bSubmenuActive[SM_Object_List]);

	static ImGuiTextFilter filter;
	ImGui::TextWrapped(lang.ReadString("Objects", "obj_list_text_copy", "Click on any entry to copy to clipboard."));
	ImGui::Text(lang.ReadString("Objects", "obj_list_search", "Search"));
	ImGui::PushItemWidth(-FLT_MIN);
	filter.Draw("");
	ImGui::PopItemWidth();

	ImGui::BeginChild("##olist", { 0, -ImGui::GetFrameHeightWithSpacing() }, true);

	for (auto &pair : object_dir)
	{
		const std::string key = pair.first;
		const std::string display_text = pair.second.translate + " [" + pair.second.original + "]";

		if (filter.PassFilter(display_text.c_str()))
		{
			bool is_selected = (selectID == key);
			if (ImGui::Selectable(display_text.c_str(), is_selected))
			{
				selectID = key;

				const size_t len = key.length() + 1;
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
				memcpy(GlobalLock(hMem), key.c_str(), len);
				GlobalUnlock(hMem);
				OpenClipboard(NULL);
				EmptyClipboard();
				SetClipboardData(CF_TEXT, hMem);
				CloseClipboard();
				Notifications->SetNotificationTime(5000);
				Notifications->PushNotification(lang.ReadString("Objects", "obj_list_notification", "Copied %s to clipboard!"));
			}
		}
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();

	ImGui::End();
}

char* int_to_str(int value, char* buffer) {
	sprintf(buffer, "%d", value);
	return buffer;
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
		CPlayer* plr = CMainGameComponent::Get()->GetPlayerManager()->GetPlayer();
		if (plr)
		{
			CThing* t = plr->GetCharacterThing();

			if (TheMenu->m_bIsActive && !TheMenu->m_bFrozeControls)
			{
				plr->SetMode(17, false);
				TheMenu->m_bFrozeControls = true;
			}
			else if (!TheMenu->m_bIsActive && TheMenu->m_bFrozeControls)
			{
				plr->ClearMode(17);
				TheMenu->m_bFrozeControls = false;
			}

			if (TheMenu->m_bGodMode)
			{
				CThing* t = plr->GetCharacterThing();
				if (t)
					t->m_fHealth = 1000.0f;
			}

			if (TheMenu->m_bInfiniteWill)
			{
				if (t)
				{
					CTCHeroStats* stats = t->GetHeroStats();
					stats->m_nWillPower = 10000.0f;
				}
				
			}

			if (TheMenu->m_bCustomCameraPos && TheMenu->ms_bFreeCam && TheMenu->m_nFreeCameraMode == FREE_CAMERA_CUSTOM)
				FreeCamera::Update();
		}

	}
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

float GetDeltaTime()
{
	float delta = 1.0f / 60.0f;

	if (eDirectX9Hook::ms_bInit)
		delta = 1.0f / ImGui::GetIO().Framerate;

	return delta;
}

bool IsWindowFocused()
{
	return TheMenu->m_bIsFocused;
}
