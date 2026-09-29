// Does the shield now face the player immediately instead of pausing to turn?
// Drive the real EnemySystem with the player jumping from one side to the other and
// watch facing on every tick.
#include "enemy_system.h"
#include "enemy_data.h"
#include <cstdio>
#include <cmath>

int main() {
    EnemySystem sys;
    sys.Clear();
    const unsigned id = sys.Spawn(EnemyKind::Shield, 1500);
    const auto& unit = [&]() -> const EnemyUnit& {
        for (const auto& u : sys.Units()) if (u.id == id) return u;
        static EnemyUnit dummy; return dummy;
    };

    Vector2 player{1120.0F, GameConfig::kFloorY};   // start on the left
    for (int i = 0; i < 30; ++i) sys.Update(1.0F / 60.0F, player);
    printf("after settling on the left   facing=%+d  (expect -1)\n", unit().facing);

    player.x = 1880.0F;                             // teleport to the right
    sys.Update(1.0F / 60.0F, player);
    printf("1 tick after crossing to the right  facing=%+d  (expect +1, no turn delay)\n",
           unit().facing);

    int ticksUntilFacing = -1;
    player.x = 1120.0F;
    for (int i = 1; i <= 120; ++i) {
        sys.Update(1.0F / 60.0F, player);
        if (unit().facing == -1) { ticksUntilFacing = i; break; }
    }
    printf("ticks to face left again after crossing back: %d  (1 = immediate)\n", ticksUntilFacing);

    int states = 0;
    printf("\nfacing while attacking (must never flip mid-attack):\n");
    player.x = 1880.0F;
    for (int i = 0; i < 40; ++i) sys.Update(1.0F / 60.0F, player);
    player.x = 1120.0F;
    int seen = unit().facing;
    for (int i = 0; i < 240; ++i) {
        sys.Update(1.0F / 60.0F, player);
        const auto& u = unit();
        if (u.state == EnemyState::Windup || u.state == EnemyState::Strike) {
            if (u.facing != seen) { printf("  FLIPPED mid-attack at tick %d\n", i); ++states; }
            seen = u.facing;
        }
    }
    printf("  mid-attack flips: %d\n", states);
    return 0;
}
