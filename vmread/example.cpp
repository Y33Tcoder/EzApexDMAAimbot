#include "hlapi/hlapi.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <random>
#include <chrono>
#include <iostream>
#include <cfloat>
#include <math.h>
#include "offsets.h"
#include "vector.h"
#include <thread>
#include <X11/Xlib.h>
#include "X11/keysym.h"
//#include <functional>



#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

FILE* dfile;

bool active = true;
uintptr_t aimentity = 0;
uintptr_t tmp_aimentity = 0;
float max = 999.0f;
int team_player = 0;
int spectators = 0;
int tmp_spectators = 0;
int playerId = 0;
int s_FOV = 7;
int toRead = 150;
//bool target_lock = false;

uint64_t g_Base;

int f_cntdown = 0;

int bone = 2;


bool key_is_pressed(KeySym ks) {
    Display *dpy = XOpenDisplay(NULL);
    char keys_return[32];
    XQueryKeymap(dpy, keys_return);
    KeyCode kc2 = XKeysymToKeycode(dpy, ks);
    bool isPressed = !!(keys_return[kc2 >> 3] & (1 << (kc2 & 7)));
    XCloseDisplay(dpy);
    return isPressed;
}

bool ctrl_is_pressed() {
    return key_is_pressed(XK_space);
}










//////////////////////////////////////////////////////////////////////////////Game.h, Game.cpp, 
////Math.h

struct SVector {
	float x;
	float y;
	float z;
	SVector(float x1, float y1, float z1) {
		x = x1;
		y = y1;
		z = z1;
	}
	SVector(QAngle q) {
		x = q.x;
		y = q.y;
		z = q.z;
	}
};

namespace Math {
	void NormalizeAngles(QAngle& angle);
	double GetFov(const QAngle& viewAngle, const QAngle& aimAngle);
	double DotProduct(const Vector& v1, const float* v2);
	QAngle CalcAngle(const Vector& src, const Vector& dst);
}



////Game .h

#define NUM_ENT_ENTRIES			(1 << 12)
#define ENT_ENTRY_MASK			(NUM_ENT_ENTRIES - 1)

struct Bone {
	uint8_t shit[0xCC];
	float x;
	uint8_t shit2[0xC];
	float y;
	uint8_t shit3[0xC];
	float z;
};
class Entity {
public:
	uint64_t ptr;
	uint8_t buffer[0x2FF0];
	Vector getPosition();
	bool isPlayer();
	int getTeamId();
	int getHealth();
	int getShield();
	int getDownState();
	QAngle GetViewAngles();
	Vector GetCamPos();
	QAngle GetRecoil();
	Vector GetViewAnglesV();

	void SetViewAngles(WinProcess& fake_i, SVector angles);
	void SetViewAngles(WinProcess& fake_i, QAngle& angles);
	Vector getBonePosition(WinProcess& fake_i, int id);
	uint64_t Observing(WinProcess& fake_i, uint64_t entitylist);

private:
	struct Bone {
		uint8_t shit[0xCC];
		float x;
		uint8_t shit2[0xC];
		float y;
		uint8_t shit3[0xC];
		float z;
	};
};
Entity getEntity(WinProcess& fake_i, uint64_t ptr);
bool WorldToScreen(Vector from, float* m_vMatrix, int targetWidth, int targetHeight, Vector& to);
//Vector GetEntityBasePosition(SOCKET sock, int pid, uintptr_t ent);
//uintptr_t GetEntityBoneArray(SOCKET sock, int pid, uintptr_t ent);
//Vector GetEntityBonePosition(SOCKET sock, int pid, uintptr_t ent, uint32_t BoneId, Vector BasePosition);
//QAngle GetViewAnglesA(SOCKET sock, int pid, uintptr_t ent);
//void SetViewAngles(SOCKET sock, int pid, uintptr_t ent, SVector angles);
//void SetViewAngles(SOCKET sock, int pid, uintptr_t ent, QAngle angles);
//Vector GetCamPos(SOCKET sock, int pid, uintptr_t ent);
float CalculateFov(Entity& from, Entity& target);
QAngle CalculateBestBoneAim(WinProcess& fake_i, Entity& from, uintptr_t target, float max_fov, int spectators);
///////////////////////////////////////////////////////////////////////////////////////////////////
//weaponxentity class

class WeaponXEntity{
public:
	//explicit WeaponXEntity(uint64_t entity_ptr);
	void update(WinProcess& fake_i, uint64_t LocalPlayer);

	float get_projectile_speed() const;
	float get_projectile_gravity() const;

	// Gets the set of desired items and attachments.
	//ItemSet get_desired_items() const;

public:
	float projectile_scale;
	float projectile_speed;
};



void WeaponXEntity::update(WinProcess& fake_i, uint64_t LocalPlayer) {

	uint64_t entitylist = g_Base + OFFSET_ENTITYLIST;
	

	uint64_t wephandle = fake_i.Read<uint64_t>(LocalPlayer + 0x1944);
	
	wephandle &= 0xffff;
	printf("Wephandle: %lx\n", wephandle);

	uint64_t wep_entity = fake_i.Read<uint64_t>(entitylist + (wephandle << 5));

	printf("Wep_entity: %lx\n", wep_entity); 



	projectile_speed = fake_i.Read<float>(wep_entity + OFFSET_BULLET_SPEED);
	projectile_scale = fake_i.Read<float>(wep_entity + OFFSET_BULLET_GRAVITY);
}
float WeaponXEntity::get_projectile_speed() const {
	return projectile_speed;
}
float WeaponXEntity::get_projectile_gravity() const {
	return /*sv_gravity*/750.0f * projectile_scale;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Math.cpp

void Math::NormalizeAngles(QAngle& angle)
{
	while (angle.x > 89.0f)
		angle.x -= 180.f;

	while (angle.x < -89.0f)
		angle.x += 180.f;

	while (angle.y > 180.f)
		angle.y -= 360.f;

	while (angle.y < -180.f)
		angle.y += 360.f;
}

QAngle Math::CalcAngle(const Vector& src, const Vector& dst)
{
	QAngle angle = QAngle();
	SVector delta = SVector((src.x - dst.x), (src.y - dst.y), (src.z - dst.z));

	double hyp = sqrt(delta.x*delta.x + delta.y * delta.y);

	angle.x = atan(delta.z / hyp) * (180.0f / M_PI);
	angle.y = atan(delta.y / delta.x) * (180.0f / M_PI);
	angle.z = 0;
	if (delta.x >= 0.0) angle.y += 180.0f;

	return angle;
}

double Math::GetFov(const QAngle& viewAngle, const QAngle& aimAngle)
{
	QAngle delta = aimAngle - viewAngle;
	NormalizeAngles(delta);

	return sqrt(pow(delta.x, 2.0f) + pow(delta.y, 2.0f));
}

double Math::DotProduct(const Vector& v1, const float* v2)
{
	return v1.x * v2[0] + v1.y * v2[1] + v1.z * v2[2];
}


///////////////////////////////////////////////////////////////////////////////////////////////////
/// Game.cpp
uint64_t Entity::Observing(WinProcess& fake_i, uint64_t entitylist) {
	
	
	uint64_t indexraw = fake_i.Read<uint64_t>(ptr + OFFSET_OBSERVING_TARGET);

	
	indexraw &= 0xffff;
	printf("rawindex=%lx\n", indexraw);
	if (indexraw != 0xffff){
		printf("Observing! \n");
		uint64_t centity2 = fake_i.Read<uint64_t>(entitylist + ((uint64_t)indexraw << 5));
		return centity2;

	}
	//int ObserverID = index;
	//if (ObserverID > 0) {
	//	printf("Obersering someone! ID=%i\n",ObserverID );
	//	uint64_t centity2 = fake_i.Read<uint64_t>(entitylist + ((uint64_t)ObserverID << 5));
	//	return centity2;
	//}
	return 0;
}
int Entity::getTeamId() {
	return *(int*)(buffer + OFFSET_TEAM);
}
int Entity::getHealth() {
	return *(int*)(buffer + OFFSET_HEALTH);
}
int Entity::getShield() {
	return *(int*)(buffer + OFFSET_SHIELD);
}
int Entity::getDownState() {
	return *(int*)(buffer + OFFSET_BLEED_OUT_STATE);
}
Vector Entity::getPosition() {
	return *(Vector*)(buffer + OFFSET_ORIGIN);
}
bool Entity::isPlayer() {
	return *(uint64_t*)(buffer + OFFSET_NAME) == 125780153691248;
}
Vector Entity::getBonePosition(WinProcess& fake_i, int id) {
	Vector position = getPosition();
	uintptr_t boneArray = *(uintptr_t*)(buffer + OFFSET_BONES);
	Vector bone = Vector();
	uint32_t boneloc = (id * 0x30);
	Bone bo = {};
	bo = fake_i.Read<Bone>(boneArray + boneloc);
	bone.x = bo.x + position.x;
	bone.y = bo.y + position.y;
	bone.z = bo.z + position.z;
	return bone;
}

QAngle Entity::GetViewAngles()
{
	return *(QAngle*)(buffer + OFFSET_VIEWANGLES);
}

Vector Entity::GetViewAnglesV()
{
	return *(Vector*)(buffer + OFFSET_VIEWANGLES);
}

void Entity::SetViewAngles(WinProcess& fake_i, SVector angles)
{
	fake_i.Write<SVector>(ptr + OFFSET_VIEWANGLES, angles);
}
void Entity::SetViewAngles(WinProcess& fake_i, QAngle& angles)
{
	SetViewAngles(fake_i, SVector(angles));
}

Vector Entity::GetCamPos()
{
	return *(Vector*)(buffer + OFFSET_CAMERAPOS);
}

QAngle Entity::GetRecoil()
{
	return *(QAngle*)(buffer + OFFSET_AIMPUNCH);
}

float CalculateFov(Entity& from, Entity& target) {
	QAngle ViewAngles = from.GetViewAngles();
	Vector LocalCamera = from.GetCamPos();
	Vector EntityPosition = target.getPosition();
	QAngle Angle = Math::CalcAngle(LocalCamera, EntityPosition);
	return Math::GetFov(ViewAngles, Angle);
}

QAngle CalculateBestBoneAim(WinProcess& fake_i, Entity& from, uintptr_t t, float max_fov, int spectators) {
	Entity target = getEntity(fake_i, t);
	int health = target.getHealth();
	if (health < 1 || health > 100) {
		printf("%s\n", "Target Invalid! (health too high or low)");
		return QAngle(0, 0, 0);
	}

	Vector EntityPosition = target.getPosition();
	Vector LocalPlayerPosition = from.getPosition();
	float dist = LocalPlayerPosition.DistTo(EntityPosition);
	if (f_cntdown == 1000){
		f_cntdown = 0;
	}


	if(f_cntdown == 0){

		int random = rand();
		printf("random:%i\n", random);
		
		
		if (random % 2 == 0){
			bone = 5;
		}
		else{
			bone = 6;
		}
	}
	++f_cntdown;

	//int bone = 2;
	//if (dist < 1200) {
	//	bone = 24; //no aim head 6=head
	//}

	Vector LocalCamera = from.GetCamPos();
	Vector BonePosition = target.getBonePosition(fake_i,bone);
	QAngle CalculatedAngles = Math::CalcAngle(LocalCamera, BonePosition);
	QAngle ViewAngles = from.GetViewAngles();
	QAngle Delta = CalculatedAngles - ViewAngles;

	double fov = Math::GetFov(ViewAngles, CalculatedAngles);
	if (fov > max_fov) {
		printf("%s\n", "FOV > MAXFOV!");
		return QAngle(0, 0, 0);
	}

	double recoildim = 0.91f + static_cast <double> (rand()) /( static_cast <double> (RAND_MAX/(0.95f-0.91f)));
	//printf("%f\n", recoildim);

	QAngle RecoilVec = from.GetRecoil();
	if (RecoilVec.x != 0 || RecoilVec.y != 0) {
		Delta -= RecoilVec * recoildim;
	}

	

	double smoothx = 45.0f + static_cast <double> (rand()) /( static_cast <double> (RAND_MAX/(55.0f-45.0f)));
	double smoothy = 40.0f + static_cast <double> (rand()) /( static_cast <double> (RAND_MAX/(50.0f-40.0f)));

	Math::NormalizeAngles(Delta);
	if (true) {
		if (Delta.x > 0.0f) {
			Delta.x /= smoothx;
		}
		else {
			Delta.x = ((Delta.x * -1L) / smoothx) * -1;
		}

		if (Delta.y > 0.0f) {
			Delta.y /= smoothy;
		}
		else {
			Delta.y = ((Delta.y * -1L) / smoothy) * -1;
		}
	}

	QAngle SmoothedAngles = ViewAngles + Delta;


	Math::NormalizeAngles(SmoothedAngles);

	return SmoothedAngles;
}

Entity getEntity(WinProcess& fake_i, uint64_t ptr) {
	Entity entity = Entity();
	entity.ptr = (uint64_t)ptr;
	fake_i.ReadMem((uint64_t)ptr, (uint64_t)entity.buffer);
	return entity;
}

bool WorldToScreen(Vector from, float* m_vMatrix, int targetWidth, int targetHeight, Vector& to)
{
	float w = m_vMatrix[12] * from.x + m_vMatrix[13] * from.y + m_vMatrix[14] * from.z + m_vMatrix[15];

	if (w < 0.01f) return false;

	to.x = m_vMatrix[0] * from.x + m_vMatrix[1] * from.y + m_vMatrix[2] * from.z + m_vMatrix[3];
	to.y = m_vMatrix[4] * from.x + m_vMatrix[5] * from.y + m_vMatrix[6] * from.z + m_vMatrix[7];

	float invw = 1.0f / w;
	to.x *= invw;
	to.y *= invw;

	float x = targetWidth / 2;
	float y = targetHeight / 2;

	x += 0.5 * to.x * targetWidth + 0.5;
	y -= 0.5 * to.y * targetHeight + 0.5;

	to.x = x;
	to.y = y;
	to.z = 0;

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


/// main.cpp imported funcs


void ProcessPlayer(WinProcess& fake_i, Entity& LPlayer, Entity& target, uint64_t entitylist) {
	if (target.Observing(fake_i, entitylist) == LPlayer.ptr) {
		tmp_spectators++;
		//system("color 4E");
	}

	//int visible = fake_i.Read<int>(target.ptr + OFFSET_IS_VISIBLE);
	//printf("%i\n", visible);

	Vector EntityPosition = target.getPosition();
	Vector LocalPlayerPosition = LPlayer.getPosition();
	float dist = LocalPlayerPosition.DistTo(EntityPosition);
	if (dist > 6000.0f) {
		printf("%s\n", "OVERDISTANCE!");
		return;
	}

	int health = target.getHealth();
	if (health < 1 || health > 100){
		printf("%s\n", "In ProcessPlayer, target health off");
		return;
	}

	int entity_team = target.getTeamId();
	if (entity_team < 0 || entity_team>50|| entity_team == team_player){
		printf("%s\n", "Entity's team is missing or is same team");
		printf("%i\n", entity_team);

		return;

	}


	float fov = CalculateFov(LPlayer, target);
	if (fov < max) {
		max = fov;
		tmp_aimentity = target.ptr;
	}
}


void DoActions(WinProcess& fake_i) {
	
	while (active)
	{
		sleep(1);

		//if some key is pressed, then active = false and break
		

		
		uint64_t LocalPlayer = fake_i.Read<uint64_t>(g_Base + OFFSET_LOCAL_ENT); //read in the self-player
		printf("LocalPlayer: %lx \n"   , LocalPlayer);
		if (LocalPlayer == 0) break;  //if no self-player is found, break
		printf("%s\n", "ok here!\n");

		Entity LPlayer = getEntity(fake_i, LocalPlayer); //get entity obj of self-player

		team_player = LPlayer.getTeamId();         // get the Team number of self-player
		printf("Team: %i\n",team_player);
		

		if (team_player < 0 || team_player>50) break; // if the player isnt in any teams, break

		uint64_t entitylist = g_Base + OFFSET_ENTITYLIST;

		uint64_t baseent = fake_i.Read<uint64_t>(entitylist); //read in BaseEntity

		printf("BaseEntity: %lx \n", baseent);

		if (baseent == 0) break; //if no BaseEntity found, break

		max = 999.0f;
		int cc = 0;
		tmp_spectators = 0;
		int lastIndexReadedCorrectly = 0;

		for (int i = 0; i <= toRead; i++) //loop through 150 entities
		{
			//printf("cc = %i\n", cc);
			uint64_t centity = fake_i.Read<uint64_t>(entitylist + ((uint64_t)i << 5));
			if (centity == 0) continue; //skip non-player entities
			if (LocalPlayer == centity) continue; //skip self-player

			Entity Target = getEntity(fake_i, centity); //create Entity obj for each entity found
			if (!Target.isPlayer()) continue;   //if target isnt player ,break
			if (Target.getDownState() != 0) continue;

			cc++;
			lastIndexReadedCorrectly = i;

			ProcessPlayer(fake_i, LPlayer, Target, entitylist);
		}
		printf("cc = %i\n", cc);
		if (toRead == 0) {
			toRead = 150;
		}
		else if (lastIndexReadedCorrectly < toRead) {
			toRead = lastIndexReadedCorrectly;
		}
		else {
			toRead += 5;
		}
		spectators = tmp_spectators;
		if (spectators > 0) {
			//system("color 4E");
		}
		else {
			//system("color 07");
		}
		printf("%d pr %d sp %d tr\n", cc, spectators, toRead);
		//uint64_t kbtn2 = fake_i.Read<uint64_t>(g_Base + 0x28268d70); //mb4
		//if (kbtn2 == 0x0){
		//	continue;
		//}
		aimentity = tmp_aimentity;
		
		
	}
}








/////////////////////////////////////////////////////////////////////////////////////////////////////
//actual example.cpp



void glowFunc(WinProcess& fake_i, uint64_t AnEntity){

	int EntityTeam = fake_i.Read<int>(AnEntity + 0x3F0);

	float r = 0.f;
	float g = 0.f;
	float b = 0.f;

	if(EntityTeam % 6 == 0){
		r = 120.f;
		g = 0.f;
		b = 60.f;
	}

	else if(EntityTeam % 5 == 0){
		r = 120.f;
		g = 0.f;
		b = 0.f;
	}
	else if(EntityTeam % 4 == 0){
		r = 120.f;
		g = 60.f;
		b = 0.f;
	}
	else if(EntityTeam % 3 == 0){
		r = 60.f;
		g = 0.f;
		b = 60.f;
	}
	else if(EntityTeam % 2 == 0){
		r = 0.f;
		g = 0.f;
		b = 120.f;
	}
	else{
		r = 0.f;
		g = 120.f;
		b = 0.f;
	}


	fake_i.Write<bool>(AnEntity + 0x390, true);
	fake_i.Write<int>(AnEntity + 0x310, 1);
	fake_i.Write<float>(AnEntity + 0x1D0, r);
	fake_i.Write<float>(AnEntity + 0x1D4, g);
	fake_i.Write<float>(AnEntity + 0x1D8, b);
	for (int offset = 0x2D0; offset <= 0x2E8; offset += 0x4){ //Setting the of the Glow at all necessary spots
		fake_i.Write<float>(AnEntity + offset, FLT_MAX); // Setting the time of the Glow to be the Max Float value so it never runs out
	}
	fake_i.Write<float>(AnEntity + 0x2FC, FLT_MAX); //Set the Distance of the Glow to Max float value so we can see a long Distance

}



void EnableGlow(WinProcess& fake_i){
	while(true){
		sleep(1);
		
		//if(kbtn == )
		for (int k = 0; k < 100; k++) { // Enumerating through the first 100 Entities in the List because thats where all players are stored
			uint64_t EntityList = g_Base + OFFSET_ENTITYLIST;

			uint64_t BaseEntity = fake_i.Read<uint64_t>(EntityList);
			//printf("%lx \n", BaseEntity);
			if (!BaseEntity){
				printf("%s\n", "NO BASE ENTITY\n");
				continue;
			}

			uint64_t AnEntity =  fake_i.Read<uint64_t>(EntityList + ((uint64_t)k << 5));
			//fprintf(out,  "\n Entity: %lx \n"   , Entity);
			//fprintf(out, "k= %i \n", k);

			if(AnEntity == 0){ //check if the entity is player
				continue;
			}


			glowFunc(fake_i, AnEntity);

			
			//int EntityHealth = i.Read<int>(Entity + 0x3E0);
			//printf("%s\n", "ok here\n");
			//uint64_t Identifier = i.Read<uint64_t>(EntityHandle);
			//printf("%s\n", "ok here2\n");
			//std::string IdentifierString = std::to_string(Identifier);

			//printf("Team: ");
			//printf("%i",EntityTeam);
			//printf("\n");
			//printf("Health: ");
			//printf("%i",EntityHealth);
			//printf("\n");

			

			//std::cout << IdentifierString.c_str() << "\n";


		}
	}
}

static void AimbotLoop(WinProcess& fake_i){
	while (active) {
		usleep(1);
		/*
		uint64_t kbtn = fake_i.Read<uint64_t>(g_Base + 0x27e79288);
		uint64_t kbtn2 = fake_i.Read<uint64_t>(g_Base + 0x28268d70); //mb4

		printf("Kbtn: %lx \n"   , kbtn);

		printf("Kbtn2: %lx \n"   , kbtn2);
		if(kbtn2 == 0x0){
			printf("%s\n", "gayyy\n");
			continue;
		}
		*/
		if (ctrl_is_pressed() == false){
			continue;
		}
		if (aimentity == 0 || g_Base == 0){
			//printf("%s\n", "no aimentity");
			//continue;
		}
		uint64_t LocalPlayer = fake_i.Read<uint64_t>(g_Base + OFFSET_LOCAL_ENT);
		if (LocalPlayer == 0) continue;
		printf("%s\n", "Got LocalPlayer!");

		//uint64_t weaponaddr = fake_i.Read<uint64_t>(LocalPlayer + 0x1944);
		//printf("Weaponaddr: %lx", weaponaddr);
		//WeaponXEntity cur_weapon;
		//cur_weapon.update(fake_i, LocalPlayer);
		//printf("Bulletspeed = %f\n", cur_weapon.get_projectile_speed());
		//printf("Bulletgrav = %f\n", cur_weapon.get_projectile_gravity());

		Entity LPlayer = getEntity(fake_i, LocalPlayer);
		//Aimbot
		QAngle Angles = CalculateBestBoneAim(fake_i, LPlayer, aimentity, s_FOV, spectators==0?tmp_spectators:spectators);
		if (Angles.x == 0 && Angles.y == 0) {
			printf("%s\n", "Angles are zero");
			continue;
		}
		LPlayer.SetViewAngles(fake_i, Angles);
		//printf("%s\n", "Setting viewangles");
	}
	
}

__attribute__((constructor))
static void init()
{
	FILE* out = stdout;
	pid_t pid;
#if (LMODE() == MODE_EXTERNAL())
	FILE* pipe = popen("pidof qemu-system-x86_64", "r");
	fscanf(pipe, "%d", &pid);
	pclose(pipe);
#else
	out = fopen("/tmp/testr.txt", "w");
	pid = getpid();
#endif
	fprintf(out, "Using Mode: %s\n", TOSTRING(LMODE));

	dfile = out;

	try {
		WinContext ctx(pid);
		ctx.processList.Refresh();
/*
		fprintf(out, "Process List:\nPID\tVIRT\t\t\tPHYS\t\tBASE\t\tNAME\n");
		for (auto& i : ctx.processList)
			fprintf(out, "%.4lx\t%.16lx\t%.9lx\t%.9lx\t%s\n", i.proc.pid, i.proc.process, i.proc.physProcess, i.proc.dirBase, i.proc.name);
*/
		for (auto& i : ctx.processList) {
			if (!strcasecmp("r5apex.exe", i.proc.name)) {
				fprintf(out, "\nLooping process %lx:\t%s\n", i.proc.pid, i.proc.name);

				PEB peb = i.GetPeb();
				sleep(1);
				short magic = i.Read<short>(peb.ImageBaseAddress);
				g_Base = peb.ImageBaseAddress;
				fprintf(out, "\tBase:\t%lx\tMagic:\t%hx (valid: %hhx)\n", peb.ImageBaseAddress, magic, (char)(magic == IMAGE_DOS_SIGNATURE));
				std::thread t1(AimbotLoop, std::ref(i)); //aimbot thread
				std::thread t2(EnableGlow, std::ref(i)); //glow thread
				DoActions(i);                            //playerlist initialization thread
				
					

					
				

			}
		}
	

	} catch (VMException& e) {
		fprintf(out, "Initialization error: %d\n", e.value);
	}


	fclose(out);
}



int main()
{
	return 0;
}
