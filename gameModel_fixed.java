package MVC.model;

import MVC.model.Tanks.*;
import MVC.model.level.Level;
import MVC.model.projectile.*;
import MVC.model.spawnInfo;

import java.awt.*;
import java.util.*;
import java.util.List;

public class gameModel {

    private final mapModel mapModel;
    private final List<tank> tanks = new ArrayList<>();
    private final List<Player> players = new ArrayList<>();
    private final List<projectile> projectiles = new ArrayList<>();
    private final List<projectile> ammoIcons = new ArrayList<>();
    private final int tileSize;
    private AmmoType selectedAmmo = AmmoType.BASIC;
    private final Rectangle[] ammoSelectorRects;
    private final int selectorWidth = 105;
    private final int selectorHeight = 50;
    private final int selectorPadding = 20;
    private boolean healthChanged = false;
    private int currentPlayerIndex = 0;
    private boolean leftArrowPressed = false;
    private boolean rightArrowPressed = false;
    public long moveStartTime;
    public long shootStartTime;
    private final long MOVE_TIME_LIMIT = 5000;
    private final long SHOOT_TIME_LIMIT = 5000;
    private final long GAME_TIME_LIMIT = 60000;
    private final long gameStartTime = System.currentTimeMillis();
    private boolean currentPlayerHasMoved = false;
    private boolean currentPlayerHasShot = false;
    private static final int STARTING_MONEY = 1000;

    // FIX: dirty flag – igaz lesz ha a tank elmozdult, a view ezzel detektálja az elavult trajectory-t
    private boolean tankPositionDirty = false;

    public gameModel(int tileSize, int screenWidth, Level level) {
        this.tileSize = tileSize;
        this.mapModel = new mapModel(level);

        ammoSelectorRects = new Rectangle[AmmoType.values().length];
        initAmmoSelector(screenWidth);

        List<spawnInfo<Point, Integer>> spawns = mapModel.getSpawnInfos();

        if (spawns.size() < 2) {
            throw new IllegalArgumentException("The level must have at least 2 spawn points!");
        }

        basicTank t1 = new basicTank(spawns.get(0).getFirst(), spawns.get(0).getSecond());
        basicTank t2 = new basicTank(spawns.get(1).getFirst(), spawns.get(1).getSecond());

        tanks.add(t1);
        tanks.add(t2);

        players.add(new Player("Player 1", t1, STARTING_MONEY));
        players.add(new Player("Player 2", t2, STARTING_MONEY));

        tanks.get(1).setSprite(MVC.model.Resources.ImageRead.loadSprite("tank1"));

        initAmmoVisuals();
        resetTurnTimers();
    }

    // --- Game time ---

    public long getGameStartTime() { return gameStartTime; }
    public long getGameTimeLimit() { return GAME_TIME_LIMIT; }

    public boolean isGameTimeExpired() {
        return (System.currentTimeMillis() - gameStartTime) >= GAME_TIME_LIMIT;
    }

    public Player getWinner() {
        Player p1 = players.get(0);
        Player p2 = players.get(1);
        if (p1.getTank().getHealth() > p2.getTank().getHealth()) return p1;
        if (p2.getTank().getHealth() > p1.getTank().getHealth()) return p2;
        return null; // draw
    }

    // --- Turn logic ---

    public boolean canShoot() {
        return isCurrentPlayersTurn() && !currentPlayerHasShot;
    }

    public boolean isMovementPhase() {
        return !currentPlayerHasMoved &&
                (System.currentTimeMillis() - moveStartTime) < MOVE_TIME_LIMIT;
    }

    public boolean isShootingPhase() {
        return currentPlayerHasMoved && !currentPlayerHasShot &&
                (System.currentTimeMillis() - shootStartTime) < SHOOT_TIME_LIMIT;
    }

    public boolean isCurrentPlayersTurn() {
        return isMovementPhase() || isShootingPhase();
    }

    public long getMoveTimeLimit() { return MOVE_TIME_LIMIT; }
    public long getShootTimeLimit() { return SHOOT_TIME_LIMIT; }

    public void resetTurnTimers() {
        moveStartTime = System.currentTimeMillis();
        shootStartTime = 0;
        currentPlayerHasMoved = false;
        currentPlayerHasShot = false;
        // FIX: kör váltáskor a trajectory biztosan elavult
        tankPositionDirty = true;
    }

    public boolean isMoveTimeExpired() {
        return (System.currentTimeMillis() - moveStartTime) > MOVE_TIME_LIMIT;
    }

    public boolean isShootTimeExpired() {
        return shootStartTime != 0 &&
                (System.currentTimeMillis() - shootStartTime) > SHOOT_TIME_LIMIT;
    }

    public void startShootTimer() {
        shootStartTime = System.currentTimeMillis();
    }

    public void switchPlayer() {
        currentPlayerIndex = (currentPlayerIndex + 1) % 2;
        resetTurnTimers();
    }

    // --- Players & tanks ---

    public Player getCurrentPlayer() {
        return players.get(currentPlayerIndex);
    }

    public tank getCurrentPlayerTank() {
        return getCurrentPlayer().getTank();
    }

    public boolean isCurrentPlayer(tank t) {
        return tanks.indexOf(t) == currentPlayerIndex;
    }

    public List<Player> getPlayers() { return players; }
    public List<tank> getTanks() { return tanks; }

    // --- Ammo selector ---

    private void initAmmoSelector(int screenWidth) {
        int startX = (screenWidth - (selectorWidth * 4 + selectorPadding * 3)) / 2;
        int y = 10;
        for (int i = 0; i < AmmoType.values().length; i++) {
            ammoSelectorRects[i] = new Rectangle(
                    startX + i * (selectorWidth + selectorPadding),
                    y,
                    selectorWidth,
                    selectorHeight
            );
        }
    }

    private void initAmmoVisuals() {
        ammoIcons.add(new basicProjectile(30, 10, new Point(0, 0), AmmoType.BASIC.getPrice()));
        ammoIcons.add(new highDamage(400, new Point(0, 0), AmmoType.HIGH_DAMAGE.getPrice()));
        ammoIcons.add(new areaDamage(40, new Point(0, 0), 125, AmmoType.AREA_DAMAGE.getPrice()));
        ammoIcons.add(new rocket(60, new Point(0, 0), AmmoType.ROCKET.getPrice()));
    }

    public List<projectile> getAmmoIcons() { return ammoIcons; }

    public AmmoType getAmmoTypeForIndex(int index) {
        if (index >= 0 && index < AmmoType.values().length) {
            return AmmoType.values()[index];
        }
        return AmmoType.BASIC;
    }

    public void selectAmmo(int index) {
        if (index >= 0 && index < AmmoType.values().length) {
            selectedAmmo = AmmoType.values()[index];
        }
    }

    public boolean isPointInAmmoSelector(Point p) {
        for (Rectangle rect : ammoSelectorRects) {
            if (rect.contains(p)) return true;
        }
        return false;
    }

    public int getAmmoSelectorIndexAt(Point p) {
        for (int i = 0; i < ammoSelectorRects.length; i++) {
            if (ammoSelectorRects[i].contains(p)) return i;
        }
        return -1;
    }

    // --- Firing ---

    public void fireFromPlayerTank(double dx, double dy, Point startPos) {
        if (!isCurrentPlayersTurn()) return;

        Player current = getCurrentPlayer();
        int cost = selectedAmmo.getPrice();
        if (!current.canAfford(cost)) return;

        current.setMoney(current.getMoney() - cost);

        if (!currentPlayerHasMoved) currentPlayerHasMoved = true;
        currentPlayerHasShot = true;

        projectile newProjectile;
        switch (selectedAmmo) {
            case HIGH_DAMAGE:
                newProjectile = new highDamage(80, startPos, selectedAmmo.getPrice());
                break;
            case AREA_DAMAGE:
                newProjectile = new areaDamage(40, startPos, 125, selectedAmmo.getPrice());
                break;
            case ROCKET:
                newProjectile = new rocket(60, startPos, selectedAmmo.getPrice());
                break;
            default:
                newProjectile = new basicProjectile(30, 10, startPos, selectedAmmo.getPrice());
        }

        newProjectile.setVelocity(dx, dy);
        projectiles.add(newProjectile);
    }

    // --- Update loop ---

    public void update() {
        healthChanged = false;

        if (!currentPlayerHasMoved) {
            if (leftArrowPressed) moveCurrentTank(-1);
            if (rightArrowPressed) moveCurrentTank(1);

            if (isMoveTimeExpired()) {
                currentPlayerHasMoved = true;
                // FIX: mozgásfázis lejártakor is elavult a trajectory
                tankPositionDirty = true;
                if (!currentPlayerHasShot) {
                    startShootTimer();
                }
            }
        }

        if (currentPlayerHasMoved && !currentPlayerHasShot) {
            if (isShootTimeExpired()) {
                switchPlayer();
            }
        }

        Iterator<projectile> it = projectiles.iterator();
        while (it.hasNext()) {
            projectile p = it.next();
            p.update();

            for (int i = 0; i < tanks.size(); i++) {
                tank t = tanks.get(i);
                if (isProjectileHittingTank(p, t)) {
                    int prevHealth = t.getHealth();
                    t.setHealth(t.getHealth() - p.getDamage());

                    if (prevHealth != t.getHealth()) {
                        healthChanged = true;
                        t.setHit(true);
                        coinIfHit();
                    }

                    it.remove();
                    break;
                }
            }

            if (isProjectileOutOfBounds(p)) {
                it.remove();
            }
        }

        if (currentPlayerHasShot && projectiles.isEmpty()) {
            switchPlayer();
        }

        if (healthChanged) {
            for (tank t : tanks) {
                t.setHit(false);
            }
        }
    }

    // --- Helpers ---

    private boolean isProjectileHittingTank(projectile p, tank t) {
        Rectangle projectileRect = new Rectangle(p.getLocation().x - 5, p.getLocation().y - 5, 10, 10);
        Rectangle tankRect = new Rectangle(t.getPosition().x, t.getPosition().y,
                t.getSprite().getWidth(), t.getSprite().getHeight());
        return projectileRect.intersects(tankRect);
    }

    private boolean isProjectileOutOfBounds(projectile p) {
        Point loc = p.getLocation();
        return loc.x < 0 || loc.x > mapModel.getWidth() * tileSize ||
                loc.y < 0 || loc.y > mapModel.getHeight() * tileSize;
    }

    public void coinIfHit() {
        if (healthChanged) {
            getCurrentPlayer().addMoney(20);
        }
    }

    public void moveCurrentTank(int dx) {
        tank current = getCurrentPlayerTank();
        Point pos = current.getPosition();

        int newX = pos.x + dx;
        int maxX = mapModel.getWidth() * tileSize - current.getSprite().getWidth();

        if (newX < 0) newX = 0;
        if (newX > maxX) newX = maxX;

        // FIX: csak akkor jelezzük dirty-nek ha tényleg mozdult
        if (newX != pos.x) {
            tankPositionDirty = true;
        }

        current.setPosition(new Point(newX, pos.y));
    }

    public void setArrowPressed(boolean left, boolean pressed) {
        if (left) leftArrowPressed = pressed;
        else rightArrowPressed = pressed;
    }

    // FIX: a view lekérdezi és reseteli a dirty flaget
    public boolean isTankPositionDirty() {
        return tankPositionDirty;
    }

    public void clearTankPositionDirty() {
        tankPositionDirty = false;
    }

    // --- Getters ---

    public boolean isHealthChanged() { return healthChanged; }
    public mapModel getMapModel() { return mapModel; }
    public List<projectile> getProjectiles() { return projectiles; }
    public AmmoType getSelectedAmmo() { return selectedAmmo; }
    public Rectangle[] getAmmoSelectorRects() { return ammoSelectorRects; }
}
