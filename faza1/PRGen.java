import java.util.Random;

public class PRGen extends Random {
    public static final int KeySizeBytes = 32;
    public static final int KeySizeBits = 256;
    private byte[] key;

    public PRGen(byte[] key) {
        if (key != null && key.length == 32) {
            this.key = new byte[32];
            System.arraycopy(key, 0, this.key, 0, 32);
        } else {
            throw new IllegalArgumentException("Invalid key");
        }
    }

    protected int next(int bits) {
        if (bits >= 1 && bits <= 32) {
            PRF prf = new PRF(this.key);
            byte[] zero = new byte[1];
            byte[] one = new byte[]{1};
            byte[] out = prf.eval(zero);
            this.key = prf.eval(one);
            int full = (out[0] & 255) << 24 | (out[1] & 255) << 16 | (out[2] & 255) << 8 | out[3] & 255;
            return bits == 32 ? full : full & (1 << bits) - 1;
        } else {
            throw new IllegalArgumentException("Must have 1<=bits<=32");
        }
    }
}
