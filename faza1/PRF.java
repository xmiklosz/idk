import java.security.InvalidKeyException;
import java.security.Key;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import javax.crypto.KeyGenerator;
import javax.crypto.Mac;
import javax.crypto.ShortBufferException;

public class PRF {
    public static final int KeySizeBits = 256;
    public static final int KeySizeBytes = 32;
    public static final int OutputSizeBits = 256;
    public static final int OutputSizeBytes = 32;
    private static final String AlgorithmName = "HmacSHA256";
    private Mac mac;

    public PRF(byte[] prfKey) {
        assert prfKey.length == 32;

        try {
            this.mac = Mac.getInstance("HmacSHA256");
            KeyGenerator keygen = KeyGenerator.getInstance("HmacSHA256");
            SecureRandom secRand = SecureRandom.getInstance("SHA1PRNG");
            secRand.setSeed(prfKey);
            keygen.init(256, secRand);
            Key key = keygen.generateKey();
            this.mac.init(key);
        } catch (NoSuchAlgorithmException x) {
            x.printStackTrace(System.err);
        } catch (InvalidKeyException x) {
            x.printStackTrace(System.err);
        }

    }

    public synchronized void eval(byte[] inBuf, int inOffset, int numBytes, byte[] outBuf, int outOffset) throws ShortBufferException {
        this.mac.update(inBuf, inOffset, numBytes);
        this.mac.doFinal(outBuf, outOffset);
    }

    public synchronized byte[] eval(byte[] val, int offset, int numBytes) {
        try {
            byte[] ret = new byte[32];
            this.eval(val, offset, numBytes, ret, 0);
            return ret;
        } catch (ShortBufferException x) {
            x.printStackTrace(System.err);
            return null;
        }
    }

    public byte[] eval(byte[] val) {
        return this.eval(val, 0, val.length);
    }

    public static void main(String[] argv) {
        byte[] k = new byte[32];

        for(int i = 0; i < 32; ++i) {
            k[i] = (byte)i;
        }

        byte[] v = new byte[57];

        for(int i = 0; i < v.length; ++i) {
            v[i] = (byte)i;
        }

        byte[] v2 = new byte[61];

        for(int i = 0; i < v2.length; ++i) {
            v2[i] = (byte)(i + 73);
        }

        PRF prf = new PRF(k);
        byte[] x = prf.eval(v);

        assert x.length == 32;

        byte[] x2 = prf.eval(v2);

        assert x2.length == 32;

        assert !x.equals(x2);

        PRF prf2 = new PRF(k);
        byte[] xAgain = prf2.eval(v);

        assert xAgain.length == 32;

        for(int i = 0; i < x.length; ++i) {
            assert x[i] == xAgain[i];
        }

        System.out.println("OK");
    }
}
