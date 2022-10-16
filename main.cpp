#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <memory>

using namespace std;

const int QueenRadius = 30;
const int QueenSpeed = 60;
const int KnightSpeed = 100;

enum Building {
    NOTHING = -1,
    MINE,
    TOWER,
    BARRACKS_KNIGHT
};

struct Point {
    double x;
    double y;
    Point(double x, double y) : x(x), y(y) { }
    Point() : Point(0, 0) { }
    double DistanceTo(Point to) {
        return sqrt(Distance2To(to));
    }
    double Distance2To(Point to) {
        return (x - to.x) * (x - to.x) + (y - to.y) * (y - to.y);
    }
    double Length() {
        return DistanceTo({ 0, 0 });
    }
    bool IsForward(Point p) {
        return Dot(p) >= 0;
    }
    Point GetPerpendicular() {
        return { -y, x };
    }
    Point GetOtherPerpendicular() {
        return { y, -x };
    }
    Point Noramalized() {
        return Point(x, y) / Length();
    }
    double Dot(Point p) {
        return x * p.x + y * p.y;
    }
    double Cross(Point p) {
        return x * p.y - y * p.x;
    }
    Point operator+ (Point const p) {
        return Point(x + p.x, y + p.y);
    }
    Point operator- (Point const p) {
        return Point(x - p.x, y - p.y);
    }
    Point operator* (double const n) {
        return Point(x * n, y * n);
    }
    Point operator/ (double const n) {
        return Point(x / n, y / n);
    }
};

struct Unit {
    Point Position;
    int Owner; // 0 = Friendly; 1 = Enemy
    int UnitType; // -1 = QUEEN, 0 = KNIGHT, 1 = ARCHER
    int Health;
};

struct SiteInfo {
    int Id;
    int GoldAmount;
    int MaxMineSize;
    int StructureType; // -1 = No structure, 0 = Goldmine, 1 = Tower, 2 = Barracks
    int Owner; // -1 = No structure, 0 = Friendly, 1 = Enemy
    int Param1;
    int Param2;
};

struct Site {
    Point point;
    int Id;
    int Radius;
    SiteInfo Info;
    Building FutureBuilding = NOTHING;

    double BuildDistanceTo(Point p) {
        return point.DistanceTo(p) - Radius - QueenRadius;
    }
};

struct Input {
    SiteInfo ReadSiteInfo() {
        SiteInfo result;
        cin >> result.Id >> result.GoldAmount >> result.MaxMineSize >> result.StructureType >> result.Owner >> result.Param1 >> result.Param2; cin.ignore();
        return result;
    }

    Site ReadSite() {
        Site result;
        cin >> result.Id >> result.point.x >> result.point.y >> result.Radius; cin.ignore();
        return result;
    }

    Unit ReadUnit() {
        Unit result;
        cin >> result.Position.x >> result.Position.y >> result.Owner >> result.UnitType >> result.Health;
        return result;
    }
} input;

struct Output {
    void WriteWaitCommand() {
        cout << "WAIT" << endl;
    }

    void WriteMoveCommand(Point to) {
        cout << "MOVE " << (int)to.x << " " << (int)to.y << endl;
    }

    void WriteBuildCommand(int side, Building building) {
        string buildString = "BUILD ";
        buildString += std::to_string(side) + " ";
        if (building == Building::MINE)
            buildString += "MINE";
        if (building == Building::TOWER)
            buildString += "TOWER";
        if (building == Building::BARRACKS_KNIGHT)
            buildString += "BARRACKS-KNIGHT";
        cout << buildString << endl;
    }

    void WriteTrainComand(vector <int> sides) {
        string trainString = "TRAIN";   
        for (int i = 0; i < sides.size(); i++)
            trainString += " " + std::to_string(sides[i]);
        cout << trainString << endl;
    }
} output;

struct Map {
    int NumSites;
    Point Queen;
    Point EnemyQueen;
    Point Center = Point(960, 500);

    map <int, Site> Sites;
    vector <Site> Towers;
    vector <Site> EnemyTowers;
    vector <Site> Barracks;
    vector <Site> EnemyBarracks;
    vector <Unit> Units;
    vector <Unit> EnemyUnits;

    void Synhronize() {
        Towers.clear();
        EnemyTowers.clear();
        Barracks.clear();
        EnemyBarracks.clear();
        Units.clear();
        EnemyUnits.clear();
        for (int i = 0; i < NumSites; i++) {
            SiteInfo siteInfo = input.ReadSiteInfo();
            Sites[siteInfo.Id].Info = siteInfo;   
            if (siteInfo.StructureType == 1 && siteInfo.Owner == 0)
                Towers.push_back(Sites[siteInfo.Id]);
            if (siteInfo.StructureType == 1 && siteInfo.Owner == 1)
                EnemyTowers.push_back(Sites[siteInfo.Id]);
            if (siteInfo.StructureType == 2 && siteInfo.Owner == 0)
                Barracks.push_back(Sites[siteInfo.Id]);
            if (siteInfo.StructureType == 2 && siteInfo.Owner == 1)
                EnemyBarracks.push_back(Sites[siteInfo.Id]);
        }

        int num_units;
        cin >> num_units; cin.ignore();
        for (int i = 0; i < num_units; i++) {
            Unit unit = input.ReadUnit();
            if (unit.Owner == 0 && unit.UnitType == -1)
                Queen = unit.Position;
            if (unit.Owner == 1 && unit.UnitType == -1)
                EnemyQueen = unit.Position;
            if (unit.Owner == 0)
                Units.push_back(unit);
            if (unit.Owner == 1)
                EnemyUnits.push_back(unit);
        }
    }

    int FindIntersectionWithSite(Point A, Point B) {
        vector <int> obstacles;
        for (auto& obstacle : Sites) {
            if ((A - B).IsForward(obstacle.second.point - B) && (B - A).IsForward(obstacle.second.point - A)) {
                if (abs((A - B).Noramalized().Cross(A - obstacle.second.point)) < obstacle.second.Radius)
                    obstacles.push_back(obstacle.first);
            }
        }
        int result = -1;
        for (auto& obstacle : obstacles) {
            if (result == -1 || Queen.DistanceTo(Sites[obstacle].point) < Queen.DistanceTo(Sites[result].point))
                result = obstacle;
        }
        return result;
    }

    Point DetourObstacle(Point from, Point to, int obstacle) {
        Point A = Sites[obstacle].point + (to - from).Noramalized().GetPerpendicular() * (Sites[obstacle].Radius + QueenRadius);
        Point B = Sites[obstacle].point + (to - from).Noramalized().GetOtherPerpendicular() * (Sites[obstacle].Radius + QueenRadius);
        Point result = A;
        if (from.DistanceTo(B) < from.DistanceTo(A))
            result = B;
        Point delta = result - from;
        if (delta.Length() < QueenSpeed)
            result = from + delta.Noramalized() * QueenSpeed;
        return result;
    }

    double TriangleDistanceToCenter(Point p1, Point p2, Point p3) {
        return p1.Distance2To(p2) + p2.Distance2To(p3) + p3.Distance2To(p1) + Center.DistanceTo((p1 + p2 + p3) / 3) * 2;
    }

    bool InMyHalfOfField(Point point) {
        if (Queen.x < Center.x)
            return point.x < Center.x;        
        else
            return point.x > Center.x;
    }
    
    Site* FindNearestEmptyField(Point point) {
        Site* result = nullptr;
        for (auto it = Sites.begin(); it != Sites.end(); it++) {
            if ((result == nullptr || point.DistanceTo(it->second.point) < point.DistanceTo(result->point)) && it->second.Info.StructureType == -1)
                result = &it->second;
        }
        return result;
    }

    vector <Site*> FindTowerTriangle() {
        vector <Site*> result;
        for (auto it1 = Sites.begin(); it1 != Sites.end(); it1++) {
            auto it2 = it1; it2++;
            for (; it2 != Sites.end(); it2++) {
                auto it3 = it2; it3++;
                for (; it3 != Sites.end(); it3++) {
                    if (InMyHalfOfField(it1->second.point) && InMyHalfOfField(it2->second.point) && InMyHalfOfField(it3->second.point)) {
                        if (result.size() == 0 || (TriangleDistanceToCenter(it1->second.point, it2->second.point, it3->second.point) < TriangleDistanceToCenter(result[0]->point, result[1]->point, result[2]->point)))
                            result = {&it1->second, &it2->second, &it3->second};
                    }
                }
            }
        }
        return result;
    }

    Site* FindNearestEnemyBarrack() {
        Site* result = nullptr;
        for (auto& barrack : EnemyBarracks) {
            if (result == nullptr || barrack.point.DistanceTo(Queen) < result->point.DistanceTo(Queen))
                result = &barrack;
        }
        return result;
    }

    Unit* FindNearestEnemy() {
        Unit* result = nullptr;
        for (auto& enemy : EnemyUnits) {
            if ((result == nullptr || Queen.DistanceTo(result->Position) > Queen.DistanceTo(enemy.Position)) && enemy.UnitType == 0)
                result = &enemy;
        }
        return result;
    }

    int TicksToAttack() {
        Unit* nearestEnemy = FindNearestEnemy();
        if (nearestEnemy != nullptr)
            return nearestEnemy->Position.DistanceTo(Queen) / KnightSpeed;
        Site* nearestEnemyBarrack = FindNearestEnemyBarrack();
        if (nearestEnemyBarrack != nullptr)
            return nearestEnemyBarrack->point.DistanceTo(Queen) / KnightSpeed + (nearestEnemyBarrack->Info.Param1 == 0 ? 4 : nearestEnemyBarrack->Info.Param1) - 1;
        return 250;
    }
} field;

struct Action {
    Point MoveTo;
    int SiteIdToBuild;
    Building BuildingType;
    int BuildingStrength;
    Point LastQueenPosition = Point(-1, -1);

    bool IsCompleted() {
        if (BuildingType != NOTHING) {
            if (BuildingType == MINE)
                return BuildingStrength <= 0 || field.Sites[SiteIdToBuild].Info.MaxMineSize == field.Sites[SiteIdToBuild].Info.Param1;
            return BuildingStrength <= 0;
        }
        else
            return field.Queen.x == MoveTo.x && field.Queen.y == MoveTo.y;
    }

    static Action MoveAction(Point to) {
        return {to, -1, NOTHING, -1};
    }

    static Action BuildMineAction(int siteId, int strength = 1) {
        return BuildAction(siteId, Building::MINE, strength);
    }

    static Action BuildTowerAction(int siteId, int strength = 1) {
        return BuildAction(siteId, Building::TOWER, strength);
    }    

    static Action BuildBarracksAction(int siteId) {
        return BuildAction(siteId, Building::BARRACKS_KNIGHT, 1);
    }

    static Action BuildAction(int siteId, Building building, int strength) {
        return {field.Sites[siteId].point, siteId, building, strength};
    }
};

struct BuildInfo {
    Building building;
    int Strength;
};

struct Controller {
    vector <Action> ActionQueue;

    bool IsWaiting() {
        while (ActionQueue.size() > 0 && ActionQueue[0].IsCompleted())
            ActionQueue.erase(ActionQueue.begin());
        bool result = ActionQueue.size() == 0;
        return ActionQueue.size() == 0;
    }

    void Process(int touchedSite) {
        if (IsWaiting()) {
            return output.WriteWaitCommand();
        }
        Action &currentAction = ActionQueue[0];
        Point queenPosition = field.Queen;
        if (currentAction.BuildingType == NOTHING) {
            int intersectionSite = field.FindIntersectionWithSite(field.Queen, currentAction.MoveTo);
            if (intersectionSite == -1)
                output.WriteMoveCommand(currentAction.MoveTo);
            else
                output.WriteMoveCommand(field.DetourObstacle(field.Queen, currentAction.MoveTo, intersectionSite));
        }
        else {
            if (touchedSite == currentAction.SiteIdToBuild) {
                currentAction.BuildingStrength--;
                output.WriteBuildCommand(touchedSite, currentAction.BuildingType);
            }
            else {
                output.WriteMoveCommand(currentAction.MoveTo);
            }
        }
        currentAction.LastQueenPosition = queenPosition;
    }

    void AddAction(Action action) {
        ActionQueue.push_back(action);
    }

    void ClearQueue() {
        ActionQueue.clear();
    }
};

enum ChangeState {
    DONT_CHANGE,
    STARTUP_STATE,
    CREATE_DEFENDING_STATE,
    DEFENCE_STATE,
    RETREAT_STATE,
};

struct State {
    Controller *controller;
    ChangeState changeState = DONT_CHANGE;
    State(Controller *controller) : controller(controller) { }
    virtual void Process() = 0;
};

struct StartupState : State {
    StartupState(Controller *controller) : State(controller) { }
    void Process() override {
        if (field.EnemyBarracks.size() > 0) {
            changeState = CREATE_DEFENDING_STATE;
            return;
        }
        if (controller->IsWaiting()) {
            controller->AddAction(Action::BuildAction(field.FindNearestEmptyField(field.Queen)->Id, MINE, 5));
        }
    }
};

struct CreateDefendingState : State {
    CreateDefendingState(Controller *controller) : State(controller) { }
    void Process() override {
        if (field.Towers.size() >= 3) {
            changeState = DEFENCE_STATE;
            return;
        }
        if (controller->IsWaiting()) {
            if (field.Barracks.size() == 0)
                controller->AddAction(Action::BuildAction(field.FindNearestEmptyField(field.Queen)->Id, BARRACKS_KNIGHT, 1));
            else
                controller->AddAction(Action::BuildAction(field.FindNearestEmptyField(field.Queen)->Id, TOWER, 4));
        }
    }
};

struct DefenceState : State {
    DefenceState(Controller *controller) : State(controller) { }
    void Process() override {
        if (field.TicksToAttack() <= 3) {
            changeState = RETREAT_STATE;
            return;
        }
        if (controller->IsWaiting()) {
            for (auto it = field.Sites.begin(); it != field.Sites.end(); it++) {
                if (it->second.Info.StructureType == TOWER && it->second.Info.Owner == 0) {
                    controller->AddAction(Action::BuildTowerAction(it->second.Id, 3));
                }
            }
        }
    }
};

struct RetreatState : State {
    RetreatState(Controller *controller) : State(controller) { }
    void Process() override {
        if (field.TicksToAttack() >= 5) {
            changeState = DEFENCE_STATE;
            return;
        }
        if (controller->IsWaiting()) {
            Site* towerToRetreat = nullptr;
            for (auto& tower : field.Towers)
                if (towerToRetreat == nullptr || field.Center.DistanceTo(tower.point) > field.Center.DistanceTo(towerToRetreat->point))
                    towerToRetreat = &tower;
            Point moveTo = towerToRetreat->point + (field.Queen - field.FindNearestEnemy()->Position).Noramalized() * (towerToRetreat->Radius + QueenRadius);
            if ((moveTo - field.Queen).Length() < 30)
                controller->AddAction(Action::BuildTowerAction(towerToRetreat->Id, 2));
            else
                controller->AddAction(Action::MoveAction(moveTo));
        }
    }
};

struct RushTowerState : State {
    RushTowerState(Controller *controller) : State(controller) { }
    void Process() override {
        if (controller->IsWaiting()) {
            vector <Site*> towers = field.FindTowerTriangle();
            controller->AddAction(Action::BuildTowerAction(towers[0]->Id, 2));
            controller->AddAction(Action::BuildTowerAction(towers[1]->Id, 2));
            controller->AddAction(Action::BuildTowerAction(towers[2]->Id, 2));
        }
    }
};

struct RushState : State {
    RushState(Controller *controller) : State(controller) { }
    void Process() override {
        controller->ClearQueue();
        if (field.EnemyBarracks.size() > 0)
            controller->AddAction(Action::BuildBarracksAction(field.EnemyBarracks[0].Id));
        else {
            
            controller->AddAction(Action::MoveAction(field.EnemyQueen));
            Site* nearestEmptyField = field.FindNearestEmptyField(field.Queen);
            if (nearestEmptyField->BuildDistanceTo(field.Queen) < QueenSpeed) {
                controller->ClearQueue();
                controller->AddAction(Action::BuildMineAction(nearestEmptyField->Id, 1));
            }
        }
    }
};

struct Strategy {
    int Turn = 0;
    int Gold;
    int TouchedSite = -1;
    int SiteIndex = 0;
    int BarracksBuilded = 0;
    Controller controller;
    unique_ptr<State> CurrentState = make_unique<StartupState>(&controller);

    void Startup() {
        cin >> field.NumSites; cin.ignore();
        for (int i = 0; i < field.NumSites; i++) {
            Site site = input.ReadSite();
            field.Sites.insert({site.Id, site});
        }
    }

    void Syncronize() {
        cin >> Gold >> TouchedSite; cin.ignore();
        field.Synhronize();
    }

    void Move() {    
        CurrentState->Process();
        if (controller.IsWaiting()) {
            CurrentState->Process();
        }
        if (CurrentState->changeState != DONT_CHANGE) {
            controller.ClearQueue();
            if (CurrentState->changeState == STARTUP_STATE)
                CurrentState = make_unique<StartupState>(&controller);
            if (CurrentState->changeState == CREATE_DEFENDING_STATE)
                CurrentState = make_unique<CreateDefendingState>(&controller);
            if (CurrentState->changeState == DEFENCE_STATE)
                CurrentState = make_unique<DefenceState>(&controller);
            if (CurrentState->changeState == RETREAT_STATE)
                CurrentState = make_unique<RetreatState>(&controller);
            CurrentState->Process();
        }
        controller.Process(TouchedSite);
    }

    void Train() { 
        vector <int> train;
        if (Gold >= 80) {
            sort(field.Barracks.begin(), field.Barracks.end(), [](Site a, Site b) {
                return field.EnemyQueen.DistanceTo(a.point) < field.EnemyQueen.DistanceTo(b.point);
            });
            if (0 < field.Barracks.size()) {            
                train.push_back(field.Barracks[0].Id);
            }   
        }
        output.WriteTrainComand(train);
    }
};

int main() {
    Strategy strategy;
    strategy.Startup();
    while (1) {
        strategy.Turn++;
        strategy.Syncronize();
        strategy.Move();
        strategy.Train();       
        cerr << field.TicksToAttack();
    }
}
