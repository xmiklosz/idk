package MVC.View;

import MVC.model.AmmoType;
import MVC.model.Player;
import MVC.model.gameModel;
import MVC.model.Tanks.tank;
import MVC.model.Resources.ImageRead;
import MVC.model.projectile.projectile;

import javax.swing.*;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.util.ArrayList;
import java.util.List;

public class gameView extends JPanel {
    private final gameModel model;
    private final int tileSize;
    private List<Point> previewTrajectory;
    private int shakeIntensity = 0;
    private int shakeOffsetX = 0;
    private int shakeOffsetY = 0;
    private Rectangle leftArrowRect;
    private Rectangle rightArrowRect;
    List<tank> tanks;

    // FIX: tároljuk a tank pozícióját amikor a trajectory-t beállítottuk
    private Point trajectoryTankPos = null;

    public gameView(gameModel model, int tileSize) {
        this.model = model;
        this.tileSize = tileSize;
        setPreferredSize(new Dimension(
                model.getMapModel().getWidth() * tileSize,
                model.getMapModel().getHeight() * tileSize
        ));
    }

    public void setShakeIntensity(int intensity) {
        this.shakeIntensity = intensity;
    }

    // FIX: eltároljuk melyik tank pozícióból számolták a trajectory-t
    public void setPreviewTrajectory(List<Point> trajectory) {
        this.previewTrajectory = trajectory;
        if (trajectory != null) {
            Point p = model.getCurrentPlayerTank().getPosition();
            trajectoryTankPos = new Point(p.x, p.y);
        } else {
            trajectoryTankPos = null;
        }
        repaint();
    }

    // FIX: külső törlési lehetőség (controller hívhatja kör váltáskor)
    public void clearPreviewTrajectory() {
        previewTrajectory = null;
        trajectoryTankPos = null;
        repaint();
    }

    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);

        // FIX: ha a model jelzi hogy a tank elmozdult, dobjuk el az elavult trajectory-t
        if (model.isTankPositionDirty()) {
            previewTrajectory = null;
            trajectoryTankPos = null;
            model.clearTankPositionDirty();
        }

        if (shakeIntensity > 0) {
            shakeOffsetX = (int) (Math.random() * shakeIntensity * 2) - shakeIntensity;
            shakeOffsetY = (int) (Math.random() * shakeIntensity * 2) - shakeIntensity;
            shakeIntensity--;
        } else {
            shakeOffsetX = 0;
            shakeOffsetY = 0;
        }

        g.translate(shakeOffsetX, shakeOffsetY);

        for (int y = 0; y < model.getMapModel().getHeight(); y++) {
            for (int x = 0; x < model.getMapModel().getWidth(); x++) {
                int tileValue = model.getMapModel().getTile(x, y);
                BufferedImage img = tile.fromInt(tileValue).getSprite();
                g.drawImage(img, x * tileSize, y * tileSize, tileSize, tileSize, null);
            }
        }

        drawTanks(g);
        drawPreviewTrajectory(g);
        drawProjectiles(g);
        drawAmmoSelector(g);
        drawArrows(g);
        drawTimers(g);
        drawCoin(g);

        g.translate(-shakeOffsetX, -shakeOffsetY);
    }

    private void drawTimers(Graphics g) {
        Graphics2D g2d = (Graphics2D) g;
        g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

        // --- Game over screen ---
        if (model.isGameTimeExpired()) {
            Player winner = model.getWinner();

            // Semi-transparent dark overlay
            g2d.setColor(new Color(0, 0, 0, 180));
            g2d.fillRect(0, 0, getWidth(), getHeight());

            String line1 = (winner != null) ? winner.getName() + " wins!" : "It's a draw!";
            String line2 = "Time's up!";

            // Winner name - large gold text
            g2d.setFont(new Font("Arial", Font.BOLD, 48));
            FontMetrics fm = g2d.getFontMetrics();
            g2d.setColor(winner != null ? new Color(255, 215, 0) : Color.WHITE);
            int x1 = (getWidth() - fm.stringWidth(line1)) / 2;
            g2d.drawString(line1, x1, getHeight() / 2 - 20);

            // "Time's up!" subtitle
            g2d.setFont(new Font("Arial", Font.PLAIN, 24));
            fm = g2d.getFontMetrics();
            g2d.setColor(Color.LIGHT_GRAY);
            int x2 = (getWidth() - fm.stringWidth(line2)) / 2;
            g2d.drawString(line2, x2, getHeight() / 2 + 30);

            return; // skip normal timers when game is over
        }

        // --- Turn timer (top left) ---
        g2d.setFont(new Font("Arial", Font.BOLD, 16));
        g2d.setColor(Color.WHITE);

        if (model.isMovementPhase()) {
            long timeLeft = model.getMoveTimeLimit() - (System.currentTimeMillis() - model.moveStartTime);
            g2d.drawString("Move: " + Math.max(0, timeLeft / 1000) + "s", 20, 30);
        } else if (model.isShootingPhase()) {
            long timeLeft = model.getShootTimeLimit() - (System.currentTimeMillis() - model.shootStartTime);
            g2d.drawString("Shoot: " + Math.max(0, timeLeft / 1000) + "s", 20, 30);
        } else if (!model.getProjectiles().isEmpty()) {
            g2d.drawString("Projectile flying...", 20, 30);
        }

        // --- Game countdown timer (top right) ---
        long elapsed = System.currentTimeMillis() - model.getGameStartTime();
        long remaining = Math.max(0, model.getGameTimeLimit() - elapsed);
        long minutes = remaining / 60000;
        long seconds = (remaining % 60000) / 1000;
        String timeText = String.format("%02d:%02d", minutes, seconds);

        int boxW = 90;
        int boxH = 35;
        int boxX = getWidth() - boxW - 10;
        int boxY = 8;

        // Background box
        g2d.setColor(new Color(20, 20, 20, 200));
        g2d.fillRoundRect(boxX, boxY, boxW, boxH, 10, 10);

        // Color: green -> yellow -> red based on time remaining
        float ratio = (float) remaining / model.getGameTimeLimit();
        Color timerColor = ratio > 0.5f
                ? new Color(80, 220, 80)
                : ratio > 0.25f
                ? new Color(240, 200, 50)
                : new Color(220, 60, 60);

        // Clock icon
        g2d.setColor(timerColor);
        g2d.setFont(new Font("Arial", Font.PLAIN, 13));
        g2d.drawString("⏱", boxX + 7, boxY + 23);

        // Time text
        g2d.setFont(new Font("Arial", Font.BOLD, 18));
        FontMetrics fm = g2d.getFontMetrics();
        g2d.setColor(timerColor);
        g2d.drawString(timeText, boxX + 28, boxY + boxH / 2 + fm.getAscent() / 2 - 2);
    }

    private void drawCoin(Graphics g) {
        List<Player> players = model.getPlayers();
        FontMetrics fm = g.getFontMetrics();
        int rectY = 50, rectW = 75, rectH = 25;
        int padding = 5;

        // --- Player 1 ---
        Player player1 = players.get(0);
        BufferedImage coinSprite1 = player1.getTank().getCoinSprite();
        int rectX1 = 10;
        int coinW = coinSprite1.getWidth(null);

        g.setColor(new Color(50, 50, 50, 200));
        g.fillRect(rectX1, rectY, rectW, rectH);
        g.drawImage(coinSprite1, rectX1, rectY, null);

        String moneyText1 = String.valueOf(player1.getMoney());
        int textX1 = rectX1 + coinW + padding;
        int textY = rectY + (rectH - fm.getHeight()) / 2 + fm.getAscent();

        g.setColor(Color.WHITE);
        g.drawString(moneyText1, textX1, textY);

        // --- Player 2 ---
        Player player2 = players.get(1);
        BufferedImage coinSprite2 = player2.getTank().getCoinSprite();
        int rectX2 = 915;
        int coinX2 = rectX2 + rectW - coinW;

        g.setColor(new Color(50, 50, 50, 200));
        g.fillRect(rectX2, rectY, rectW, rectH);
        g.drawImage(coinSprite2, coinX2, rectY, null);

        String moneyText2 = String.valueOf(player2.getMoney());
        int textWidth2 = fm.stringWidth(moneyText2);
        int textX2 = coinX2 - padding - textWidth2;

        g.setColor(Color.WHITE);
        g.drawString(moneyText2, textX2, textY);
    }

    private void drawAmmoSelector(Graphics g) {
        Graphics2D g2d = (Graphics2D) g;
        AmmoType selected = model.getSelectedAmmo();
        Rectangle[] rects = model.getAmmoSelectorRects();
        List<projectile> ammoIcons = model.getAmmoIcons();
        BufferedImage coinSprite = model.getPlayers().get(0).getTank().getCoinSprite();

        for (int i = 0; i < rects.length; i++) {
            Rectangle rect = rects[i];
            rect.setSize(new Dimension(110, 50));
            projectile icon = ammoIcons.get(i);
            AmmoType type = model.getAmmoTypeForIndex(i);

            g2d.setColor(new Color(50, 50, 50, 200));
            g2d.fill(rect);

            if (type == selected) {
                g2d.setColor(Color.WHITE);
                g2d.setStroke(new BasicStroke(3));
                g2d.draw(rect);
            } else {
                g2d.setColor(new Color(88, 31, 31));
                g2d.setStroke(new BasicStroke(1));
                g2d.draw(rect);
            }

            BufferedImage sprite = icon.getSprite();
            if (sprite != null && coinSprite != null) {
                int iconSize = 20;
                int iconX = rect.x + 5;
                int iconY = rect.y + 2;
                g2d.drawImage(sprite, iconX, iconY, iconSize, iconSize, null);
                g2d.drawImage(coinSprite, iconX, iconY + iconSize + 5, iconSize, iconSize, null);
            }

            g2d.setColor(type.getColor());
            g2d.setFont(new Font("Arial", Font.BOLD, 12));

            FontMetrics fm = g2d.getFontMetrics();
            String text = type.getName();
            String priceText = "" + icon.getPrice();
            int textX = rect.x + 30;
            int textY = rect.y + 5 + fm.getAscent();

            g2d.drawString(text, textX, textY);
            g2d.drawString(priceText, textX, textY + 25);
        }
    }

    private void drawTanks(Graphics g) {
        Graphics2D g2d = (Graphics2D) g;
        tanks = model.getTanks();
        for (tank t : tanks) {
            Point pos = t.getPosition();
            BufferedImage sprite = t.getSprite();
            g2d.drawImage(sprite, pos.x, pos.y, null);
            drawHealthBar(g2d, t, pos);
        }
    }

    public Rectangle getLeftArrowRect() { return leftArrowRect; }
    public Rectangle getRightArrowRect() { return rightArrowRect; }

    private void drawArrows(Graphics g) {
        BufferedImage rightArrow = ImageRead.loadSprite("ArrowRight");
        BufferedImage leftArrow = ImageRead.loadSprite("ArrowLeft");

        if (rightArrow == null || leftArrow == null) return;

        int arrowY = getHeight() - 50;
        int arrowWidth = 40;
        int arrowHeight = 30;

        int leftArrowX = 50;
        g.drawImage(leftArrow, leftArrowX, arrowY, arrowWidth, arrowHeight, null);
        leftArrowRect = new Rectangle(leftArrowX, arrowY, arrowWidth, arrowHeight);

        int rightArrowX = getWidth() - 90;
        g.drawImage(rightArrow, rightArrowX, arrowY, arrowWidth, arrowHeight, null);
        rightArrowRect = new Rectangle(rightArrowX, arrowY, arrowWidth, arrowHeight);
    }

    private void drawHealthBar(Graphics2D g2d, tank t, Point tankPosition) {
        int barWidth = 50;
        int barHeight = 5;
        int barOffset = 10;

        int shakeOffset = 0;
        if (t.isHit()) {
            shakeOffset = (int) (Math.random() * 5) - 2;
        }

        int health = t.getHealth();
        String healthText = "" + health;
        int barX = tankPosition.x + (t.getSprite().getWidth() - barWidth) / 2 + shakeOffset;
        int barY = tankPosition.y - barOffset + shakeOffset;

        g2d.setColor(Color.RED);
        g2d.fillRect(barX, barY, barWidth, barHeight);

        double healthPercent = (double) t.getHealth() / t.getMaxHealth();
        int fillWidth = (int) (barWidth * healthPercent);
        g2d.setColor(healthPercent > 0.5 ? Color.GREEN :
                healthPercent > 0.25 ? Color.YELLOW : Color.ORANGE);
        g2d.fillRect(barX, barY, fillWidth, barHeight);

        g2d.setColor(Color.BLACK);
        g2d.drawString(healthText, barX, barY);
        g2d.drawRect(barX, barY, barWidth, barHeight);
    }

    private void drawPreviewTrajectory(Graphics g) {
        // FIX 1: ne rajzoljon ha nincs aktív kör
        if (!model.isCurrentPlayersTurn()) {
            previewTrajectory = null;
            trajectoryTankPos = null;
            return;
        }

        if (previewTrajectory == null || previewTrajectory.size() < 2) return;

        tank currentTank = model.getCurrentPlayerTank();
        Point currentTankPos = currentTank.getPosition();

        // FIX 2: ha a tank elmozdult azóta hogy a trajectory-t kiszámoltuk, dobjuk el
        if (trajectoryTankPos != null &&
                (trajectoryTankPos.x != currentTankPos.x || trajectoryTankPos.y != currentTankPos.y)) {
            previewTrajectory = null;
            trajectoryTankPos = null;
            return;
        }

        Point tankTop = new Point(
                currentTankPos.x + currentTank.getSprite().getWidth() / 2,
                currentTankPos.y
        );

        Graphics2D g2d = (Graphics2D) g.create();
        try {
            g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
            float[] dash = {8f, 8f};
            g2d.setStroke(new BasicStroke(2f, BasicStroke.CAP_BUTT, BasicStroke.JOIN_MITER, 10f, dash, 0f));
            g2d.setColor(Color.RED);

            List<Point> fullTraj = new ArrayList<>();
            fullTraj.add(tankTop);
            fullTraj.addAll(previewTrajectory);

            int n = fullTraj.size();
            int[] xs = new int[n];
            int[] ys = new int[n];
            for (int i = 0; i < n; i++) {
                xs[i] = fullTraj.get(i).x;
                ys[i] = fullTraj.get(i).y;
            }
            g2d.drawPolyline(xs, ys, n);
        } finally {
            g2d.dispose();
        }
    }

    private void drawProjectiles(Graphics g) {
        Graphics2D g2d = (Graphics2D) g;
        for (projectile p : model.getProjectiles()) {
            Point loc = p.getLocation();
            g2d.setColor(Color.BLACK);
            g2d.fillOval(loc.x - 5, loc.y - 5, 10, 10);
        }
    }

    public int getTileSize() { return tileSize; }
}
