// AI: Implementácia vytvorená s pomocou Claude AI (Anthropic) - GUI vizualizácia blockchainu (bonus).

import javax.swing.*;
import javax.swing.border.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.*;
import java.security.SignatureException;
import java.util.*;

/**
 * GUI vizualizačný nástroj pre blockchain (Fáza 3 - bonus 2 body).
 * Zobrazuje stromovú štruktúru blockchainu vrátane forkov,
 * detaily blokov a transakcií, a umožňuje interaktívne prehliadanie.
 */
public class BlockchainGUI extends JFrame {

    // --- Dátové štruktúry pre vizualizáciu ---

    /** Vizuálny uzol blockchainu */
    static class VisualBlock {
        Block block;
        String label;
        String miner;
        int height;
        VisualBlock parent;
        ArrayList<VisualBlock> children = new ArrayList<VisualBlock>();
        ArrayList<Transaction> transactions = new ArrayList<Transaction>();

        // Pozícia na plátne
        int x, y;
        int col;

        VisualBlock(Block block, String label, String miner, int height, VisualBlock parent) {
            this.block = block;
            this.label = label;
            this.miner = miner;
            this.height = height;
            this.parent = parent;
            if (parent != null) {
                parent.children.add(this);
            }
        }
    }

    // --- Konštanty GUI ---
    static final int BLOCK_W = 140;
    static final int BLOCK_H = 60;
    static final int H_GAP = 40;
    static final int V_GAP = 80;
    static final int MARGIN = 40;

    static final Color COLOR_GENESIS = new Color(255, 200, 60);
    static final Color COLOR_MAIN = new Color(100, 180, 255);
    static final Color COLOR_FORK = new Color(255, 130, 130);
    static final Color COLOR_SELECTED = new Color(130, 255, 150);
    static final Color COLOR_BG = new Color(30, 30, 40);
    static final Color COLOR_LINE = new Color(180, 180, 200);
    static final Color COLOR_TEXT = new Color(230, 230, 240);

    // --- Stav ---
    private ArrayList<VisualBlock> allBlocks = new ArrayList<VisualBlock>();
    private VisualBlock selectedBlock = null;
    private VisualBlock maxHeightBlock = null;
    private Set<VisualBlock> mainChain = new HashSet<VisualBlock>();

    private BlockchainPanel treePanel;
    private JTextArea detailArea;
    private JLabel statusBar;

    // --- Konštrukcia ---
    public BlockchainGUI() {
        super("Blockchain Vizualizácia - DMBLOCK Fáza 3");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(1100, 700);
        setLocationRelativeTo(null);

        buildBlockchain();
        computeMainChain();
        assignPositions();

        initUI();
        if (!allBlocks.isEmpty()) {
            selectBlock(allBlocks.get(0));
        }
    }

    // --- Vybudovanie demo blockchainu ---
    private void buildBlockchain() {
        try {
            byte[] key_bob = new byte[32];
            byte[] key_alice = new byte[32];
            byte[] key_cyril = new byte[32];
            for (int i = 0; i < 32; i++) {
                key_bob[i] = (byte) 1;
                key_alice[i] = (byte) 0;
                key_cyril[i] = (byte) 2;
            }
            RSAKeyPair pk_bob = new RSAKeyPair(new PRGen(key_bob), 265);
            RSAKeyPair pk_alice = new RSAKeyPair(new PRGen(key_alice), 265);
            RSAKeyPair pk_cyril = new RSAKeyPair(new PRGen(key_cyril), 265);

            Blockchain blockchain = buildDemo(pk_bob, pk_alice, pk_cyril);

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private Blockchain buildDemo(RSAKeyPair pk_bob, RSAKeyPair pk_alice, RSAKeyPair pk_cyril)
            throws SignatureException {

        // Genesis
        Block genesisBlock = new Block(null, pk_bob.getPublicKey());
        genesisBlock.finalize();
        Blockchain blockchain = new Blockchain(genesisBlock);
        HandleBlocks hb = new HandleBlocks(blockchain);

        VisualBlock vGenesis = new VisualBlock(genesisBlock, "Genesis", "Bob", 1, null);
        allBlocks.add(vGenesis);

        // Block 1: Alice ťaží, Bob -> Alice (3 výstupy)
        Block block1 = new Block(genesisBlock.getHash(), pk_alice.getPublicKey());
        Tx tx1 = new Tx();
        tx1.addInput(genesisBlock.getCoinbase().getHash(), 0);
        tx1.addOutput(1.0, pk_alice.getPublicKey());
        tx1.addOutput(1.0, pk_alice.getPublicKey());
        tx1.addOutput(1.125, pk_alice.getPublicKey());
        tx1.signTx(pk_bob.getPrivateKey(), 0);
        block1.transactionAdd(tx1);
        block1.finalize();
        hb.blockProcess(block1);

        VisualBlock v1 = new VisualBlock(block1, "Block 1", "Alice", 2, vGenesis);
        v1.transactions.add(tx1);
        allBlocks.add(v1);

        // Block 2: Bob ťaží alternatívny blok (fork od genesis)
        Block block2 = new Block(genesisBlock.getHash(), pk_bob.getPublicKey());
        Tx tx2 = new Tx();
        tx2.addInput(genesisBlock.getCoinbase().getHash(), 0);
        tx2.addOutput(1.0, pk_bob.getPublicKey());
        tx2.addOutput(1.0, pk_bob.getPublicKey());
        tx2.addOutput(1.125, pk_bob.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);
        block2.transactionAdd(tx2);
        block2.finalize();
        hb.blockProcess(block2);

        VisualBlock v2 = new VisualBlock(block2, "Block 2", "Bob", 2, vGenesis);
        v2.transactions.add(tx2);
        allBlocks.add(v2);

        // Block 3: Bob ťaží na block1, Alice -> Cyril
        Block block3 = new Block(block1.getHash(), pk_bob.getPublicKey());
        Tx tx3 = new Tx();
        tx3.addInput(tx1.getHash(), 0);
        tx3.addInput(tx1.getHash(), 1);
        tx3.addOutput(2.0, pk_cyril.getPublicKey());
        tx3.signTx(pk_alice.getPrivateKey(), 0);
        tx3.signTx(pk_alice.getPrivateKey(), 1);
        block3.transactionAdd(tx3);
        block3.finalize();
        hb.blockProcess(block3);

        VisualBlock v3 = new VisualBlock(block3, "Block 3", "Bob", 3, v1);
        v3.transactions.add(tx3);
        allBlocks.add(v3);

        // Block 4: Bob ťaží na block3, Cyril -> Bob
        Block block4 = new Block(block3.getHash(), pk_bob.getPublicKey());
        Tx tx4 = new Tx();
        tx4.addInput(tx3.getHash(), 0);
        tx4.addOutput(1.5, pk_bob.getPublicKey());
        tx4.addOutput(0.5, pk_bob.getPublicKey());
        tx4.signTx(pk_cyril.getPrivateKey(), 0);
        block4.transactionAdd(tx4);
        block4.finalize();
        hb.blockProcess(block4);

        VisualBlock v4 = new VisualBlock(block4, "Block 4", "Bob", 4, v3);
        v4.transactions.add(tx4);
        allBlocks.add(v4);

        // Block 5: Alice ťaží na block2 (fork pokračuje)
        Block block5 = new Block(block2.getHash(), pk_alice.getPublicKey());
        Tx tx5 = new Tx();
        tx5.addInput(tx2.getHash(), 0);
        tx5.addOutput(0.8, pk_alice.getPublicKey());
        tx5.signTx(pk_bob.getPrivateKey(), 0);
        block5.transactionAdd(tx5);
        block5.finalize();
        hb.blockProcess(block5);

        VisualBlock v5 = new VisualBlock(block5, "Block 5", "Alice", 3, v2);
        v5.transactions.add(tx5);
        allBlocks.add(v5);

        return blockchain;
    }

    // --- Výpočet hlavného reťazca ---
    private void computeMainChain() {
        maxHeightBlock = null;
        for (VisualBlock vb : allBlocks) {
            if (maxHeightBlock == null || vb.height > maxHeightBlock.height) {
                maxHeightBlock = vb;
            }
        }
        mainChain.clear();
        VisualBlock cur = maxHeightBlock;
        while (cur != null) {
            mainChain.add(cur);
            cur = cur.parent;
        }
    }

    // --- Priradenie pozícií blokom (stromový layout) ---
    private void assignPositions() {
        // Zoskup bloky podľa výšky
        HashMap<Integer, ArrayList<VisualBlock>> byHeight = new HashMap<Integer, ArrayList<VisualBlock>>();
        int maxH = 0;
        for (VisualBlock vb : allBlocks) {
            if (!byHeight.containsKey(vb.height)) {
                byHeight.put(vb.height, new ArrayList<VisualBlock>());
            }
            byHeight.get(vb.height).add(vb);
            if (vb.height > maxH) maxH = vb.height;
        }

        for (int h = 1; h <= maxH; h++) {
            ArrayList<VisualBlock> row = byHeight.get(h);
            if (row == null) continue;
            for (int i = 0; i < row.size(); i++) {
                VisualBlock vb = row.get(i);
                vb.col = i;
                vb.x = MARGIN + h * (BLOCK_W + H_GAP);
                vb.y = MARGIN + i * (BLOCK_H + V_GAP);
            }
        }
    }

    // --- Inicializácia UI ---
    private void initUI() {
        setLayout(new BorderLayout(5, 5));
        getContentPane().setBackground(COLOR_BG);

        // Horný panel s nadpisom
        JPanel topPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        topPanel.setBackground(new Color(40, 40, 55));
        topPanel.setBorder(new EmptyBorder(8, 15, 8, 15));
        JLabel title = new JLabel("Blockchain Vizualizacia");
        title.setForeground(COLOR_TEXT);
        title.setFont(new Font("SansSerif", Font.BOLD, 18));
        topPanel.add(title);

        JLabel info = new JLabel("    Klikni na blok pre detaily  |  Zelena = hlavny retazec  |  Cervena = fork");
        info.setForeground(new Color(160, 160, 180));
        info.setFont(new Font("SansSerif", Font.PLAIN, 12));
        topPanel.add(info);
        add(topPanel, BorderLayout.NORTH);

        // Stredný panel s vizualizáciou
        treePanel = new BlockchainPanel();
        treePanel.setPreferredSize(new Dimension(
            MARGIN * 2 + (getMaxHeight() + 1) * (BLOCK_W + H_GAP),
            MARGIN * 2 + getMaxWidth() * (BLOCK_H + V_GAP)
        ));
        JScrollPane scrollPane = new JScrollPane(treePanel);
        scrollPane.getViewport().setBackground(COLOR_BG);
        scrollPane.setBorder(BorderFactory.createLineBorder(new Color(60, 60, 80)));
        add(scrollPane, BorderLayout.CENTER);

        // Pravý panel s detailmi
        detailArea = new JTextArea(20, 35);
        detailArea.setEditable(false);
        detailArea.setBackground(new Color(35, 35, 50));
        detailArea.setForeground(COLOR_TEXT);
        detailArea.setFont(new Font("Monospaced", Font.PLAIN, 12));
        detailArea.setBorder(new EmptyBorder(10, 10, 10, 10));
        detailArea.setLineWrap(true);
        detailArea.setWrapStyleWord(true);

        JScrollPane detailScroll = new JScrollPane(detailArea);
        detailScroll.setBorder(BorderFactory.createTitledBorder(
            BorderFactory.createLineBorder(new Color(80, 80, 100)),
            " Detail bloku ",
            TitledBorder.LEFT, TitledBorder.TOP,
            new Font("SansSerif", Font.BOLD, 12),
            COLOR_TEXT
        ));
        detailScroll.getViewport().setBackground(new Color(35, 35, 50));
        add(detailScroll, BorderLayout.EAST);

        // Spodný stavový riadok
        statusBar = new JLabel("  Blokov: " + allBlocks.size()
            + "  |  Max vyska: " + (maxHeightBlock != null ? maxHeightBlock.height : 0)
            + "  |  Forky: " + countForks());
        statusBar.setForeground(new Color(160, 160, 180));
        statusBar.setFont(new Font("SansSerif", Font.PLAIN, 11));
        statusBar.setBackground(new Color(40, 40, 55));
        statusBar.setOpaque(true);
        statusBar.setBorder(new EmptyBorder(5, 10, 5, 10));
        add(statusBar, BorderLayout.SOUTH);
    }

    private int getMaxHeight() {
        int max = 0;
        for (VisualBlock vb : allBlocks) {
            if (vb.height > max) max = vb.height;
        }
        return max;
    }

    private int getMaxWidth() {
        HashMap<Integer, Integer> counts = new HashMap<Integer, Integer>();
        for (VisualBlock vb : allBlocks) {
            counts.put(vb.height, counts.getOrDefault(vb.height, 0) + 1);
        }
        int max = 1;
        for (int c : counts.values()) {
            if (c > max) max = c;
        }
        return max;
    }

    private int countForks() {
        int forks = 0;
        for (VisualBlock vb : allBlocks) {
            if (vb.children.size() > 1) forks++;
        }
        return forks;
    }

    // --- Výber bloku ---
    private void selectBlock(VisualBlock vb) {
        selectedBlock = vb;
        treePanel.repaint();
        updateDetail(vb);
    }

    private void updateDetail(VisualBlock vb) {
        StringBuilder sb = new StringBuilder();
        sb.append("=== ").append(vb.label).append(" ===\n\n");
        sb.append("Vyska:    ").append(vb.height).append("\n");
        sb.append("Taziar:   ").append(vb.miner).append("\n");

        boolean isMain = mainChain.contains(vb);
        sb.append("Retazec:  ").append(isMain ? "HLAVNY" : "FORK").append("\n");

        if (vb.block.getHash() != null) {
            sb.append("Hash:     ").append(hashToHex(vb.block.getHash(), 16)).append("\n");
        }
        if (vb.block.getPrevBlockHash() != null) {
            sb.append("Predch.:  ").append(hashToHex(vb.block.getPrevBlockHash(), 16)).append("\n");
        } else {
            sb.append("Predch.:  (ziadny - genesis)\n");
        }

        // Coinbase
        Transaction cb = vb.block.getCoinbase();
        sb.append("\n--- Coinbase ---\n");
        sb.append("Odmena:   ").append(Block.COINBASE).append(" BTC\n");
        if (cb.getHash() != null) {
            sb.append("Tx hash:  ").append(hashToHex(cb.getHash(), 16)).append("\n");
        }

        // Transakcie
        sb.append("\n--- Transakcie (").append(vb.transactions.size()).append(") ---\n");
        for (int t = 0; t < vb.transactions.size(); t++) {
            Transaction tx = vb.transactions.get(t);
            sb.append("\nTx ").append(t + 1).append(":\n");
            if (tx.getHash() != null) {
                sb.append("  Hash: ").append(hashToHex(tx.getHash(), 16)).append("\n");
            }
            sb.append("  Vstupy:  ").append(tx.numInputs()).append("\n");
            for (int i = 0; i < tx.numInputs(); i++) {
                Transaction.Input in = tx.getInput(i);
                sb.append("    [").append(i).append("] prevTx: ");
                if (in.prevTxHash != null) {
                    sb.append(hashToHex(in.prevTxHash, 8));
                }
                sb.append(" idx:").append(in.outputIndex).append("\n");
            }
            sb.append("  Vystupy: ").append(tx.numOutputs()).append("\n");
            for (int i = 0; i < tx.numOutputs(); i++) {
                Transaction.Output out = tx.getOutput(i);
                sb.append("    [").append(i).append("] ").append(out.value).append(" BTC\n");
            }
        }

        // Deti
        sb.append("\n--- Struktura ---\n");
        sb.append("Rodic:  ").append(vb.parent != null ? vb.parent.label : "(ziadny)").append("\n");
        sb.append("Deti:   ");
        if (vb.children.isEmpty()) {
            sb.append("(ziadne - list)");
        } else {
            for (int i = 0; i < vb.children.size(); i++) {
                if (i > 0) sb.append(", ");
                sb.append(vb.children.get(i).label);
            }
        }
        sb.append("\n");

        detailArea.setText(sb.toString());
        detailArea.setCaretPosition(0);
    }

    private String hashToHex(byte[] hash, int maxBytes) {
        StringBuilder sb = new StringBuilder();
        int len = Math.min(hash.length, maxBytes);
        for (int i = 0; i < len; i++) {
            sb.append(String.format("%02x", hash[i] & 0xFF));
        }
        if (hash.length > maxBytes) sb.append("...");
        return sb.toString();
    }

    // --- Panel na kreslenie blockchainu ---
    class BlockchainPanel extends JPanel {

        BlockchainPanel() {
            setBackground(COLOR_BG);
            addMouseListener(new MouseAdapter() {
                @Override
                public void mouseClicked(MouseEvent e) {
                    for (VisualBlock vb : allBlocks) {
                        if (e.getX() >= vb.x && e.getX() <= vb.x + BLOCK_W
                            && e.getY() >= vb.y && e.getY() <= vb.y + BLOCK_H) {
                            selectBlock(vb);
                            return;
                        }
                    }
                }
            });
            // Tooltip
            setToolTipText("");
        }

        @Override
        public String getToolTipText(MouseEvent e) {
            for (VisualBlock vb : allBlocks) {
                if (e.getX() >= vb.x && e.getX() <= vb.x + BLOCK_W
                    && e.getY() >= vb.y && e.getY() <= vb.y + BLOCK_H) {
                    return vb.label + " (vyska " + vb.height + ", taziar: " + vb.miner + ")";
                }
            }
            return null;
        }

        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g);
            Graphics2D g2 = (Graphics2D) g;
            g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
            g2.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);

            // Kresli spojenia
            g2.setStroke(new BasicStroke(2.5f));
            for (VisualBlock vb : allBlocks) {
                if (vb.parent != null) {
                    boolean bothMain = mainChain.contains(vb) && mainChain.contains(vb.parent);
                    g2.setColor(bothMain ? new Color(80, 200, 120, 180) : new Color(200, 100, 100, 150));
                    int x1 = vb.parent.x + BLOCK_W;
                    int y1 = vb.parent.y + BLOCK_H / 2;
                    int x2 = vb.x;
                    int y2 = vb.y + BLOCK_H / 2;

                    // Bezierova krivka
                    int cx = (x1 + x2) / 2;
                    CubicCurve2D curve = new CubicCurve2D.Double(
                        x1, y1, cx, y1, cx, y2, x2, y2);
                    g2.draw(curve);

                    // Sipka
                    drawArrow(g2, cx, y2, x2, y2);
                }
            }

            // Kresli bloky
            for (VisualBlock vb : allBlocks) {
                drawBlock(g2, vb);
            }
        }

        private void drawBlock(Graphics2D g2, VisualBlock vb) {
            Color fill;
            if (vb == selectedBlock) {
                fill = COLOR_SELECTED;
            } else if (vb.parent == null) {
                fill = COLOR_GENESIS;
            } else if (mainChain.contains(vb)) {
                fill = COLOR_MAIN;
            } else {
                fill = COLOR_FORK;
            }

            // Tieň
            g2.setColor(new Color(0, 0, 0, 60));
            g2.fillRoundRect(vb.x + 3, vb.y + 3, BLOCK_W, BLOCK_H, 12, 12);

            // Blok
            g2.setColor(fill);
            g2.fillRoundRect(vb.x, vb.y, BLOCK_W, BLOCK_H, 12, 12);

            // Okraj
            g2.setColor(fill.darker());
            g2.setStroke(new BasicStroke(2f));
            g2.drawRoundRect(vb.x, vb.y, BLOCK_W, BLOCK_H, 12, 12);

            // Text
            g2.setColor(new Color(20, 20, 30));
            g2.setFont(new Font("SansSerif", Font.BOLD, 13));
            FontMetrics fm = g2.getFontMetrics();
            int textW = fm.stringWidth(vb.label);
            g2.drawString(vb.label, vb.x + (BLOCK_W - textW) / 2, vb.y + 22);

            g2.setFont(new Font("SansSerif", Font.PLAIN, 11));
            String info = "H:" + vb.height + " | " + vb.miner;
            fm = g2.getFontMetrics();
            textW = fm.stringWidth(info);
            g2.drawString(info, vb.x + (BLOCK_W - textW) / 2, vb.y + 38);

            String txInfo = vb.transactions.size() + " tx | " + Block.COINBASE + " BTC";
            fm = g2.getFontMetrics();
            textW = fm.stringWidth(txInfo);
            g2.drawString(txInfo, vb.x + (BLOCK_W - textW) / 2, vb.y + 53);
        }

        private void drawArrow(Graphics2D g2, int x1, int y1, int x2, int y2) {
            double angle = Math.atan2(y2 - y1, x2 - x1);
            int arrowLen = 10;
            int ax1 = (int) (x2 - arrowLen * Math.cos(angle - Math.PI / 6));
            int ay1 = (int) (y2 - arrowLen * Math.sin(angle - Math.PI / 6));
            int ax2 = (int) (x2 - arrowLen * Math.cos(angle + Math.PI / 6));
            int ay2 = (int) (y2 - arrowLen * Math.sin(angle + Math.PI / 6));
            g2.fillPolygon(new int[]{x2, ax1, ax2}, new int[]{y2, ay1, ay2}, 3);
        }
    }

    // --- Pomocná Tx trieda ---
    public static class Tx extends Transaction {
        public void signTx(RSAKey sk, int input) throws SignatureException {
            byte[] sig = sk.sign(this.getDataToSign(input));
            this.addSignature(sig, input);
            this.finalize();
        }
    }

    // --- Main ---
    public static void main(String[] args) {
        SwingUtilities.invokeLater(new Runnable() {
            @Override
            public void run() {
                BlockchainGUI gui = new BlockchainGUI();
                gui.setVisible(true);
            }
        });
    }
}
