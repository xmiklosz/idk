import java.math.BigInteger;

public class RSAKey {
    public static final int NUM_ZERO_BYTES = 16;
    public static final int NUM_RANDOM_BYTES = 16;
    private static final byte MARKER_BYTE = 1;
    private static PRGen gen;
    private static PRF h;
    private final BigInteger exponent;
    private final BigInteger modulus;

    static {
        new TrueRandomness();
        byte[] randomness = TrueRandomness.get();
        gen = new PRGen(pad(randomness, 32, false));
        byte[] key = new byte[32];
        h = new PRF(key);
    }

    public RSAKey(BigInteger theExponent, BigInteger theModulus) {
        if (theExponent != null && theModulus != null) {
            this.exponent = theExponent;
            this.modulus = theModulus;
        } else {
            throw new NullPointerException();
        }
    }

    public BigInteger getExponent() {
        return this.exponent;
    }

    public BigInteger getModulus() {
        return this.modulus;
    }

    public byte[] encrypt(byte[] plaintext) {
        if (plaintext == null) {
            throw new NullPointerException();
        } else if (this.maxPlaintextLength() == 0) {
            throw new IllegalArgumentException("modulus is too small for any plaintext");
        } else if (plaintext.length > this.maxPlaintextLength()) {
            throw new IllegalArgumentException("plaintext can be at most " + this.maxPlaintextLength() + " bytes");
        } else {
            byte[] oaepOutput = this.oaep(plaintext);
            BigInteger oaepOutputBI = new BigInteger(1, oaepOutput);
            BigInteger encrypted = oaepOutputBI.modPow(this.exponent, this.modulus);
            return encrypted.toByteArray();
        }
    }

    public byte[] decrypt(byte[] ciphertext) {
        if (ciphertext == null) {
            throw new NullPointerException();
        } else if (this.maxPlaintextLength() == 0) {
            throw new IllegalArgumentException("modulus is too small for any plaintext");
        } else {
            BigInteger encrypted = new BigInteger(1, ciphertext);
            BigInteger oaepInputBI = encrypted.modPow(this.exponent, this.modulus);
            return this.oaepReverse(oaepInputBI.toByteArray());
        }
    }

    public byte[] sign(byte[] message) {
        if (message == null) {
            throw new NullPointerException();
        } else if (this.maxPlaintextLength() < 32) {
            throw new IllegalArgumentException("modulus is too small for a digital signature");
        } else {
            byte[] hashed = h.eval(message);
            return this.encrypt(hashed);
        }
    }

    public boolean verifySignature(byte[] message, byte[] signature) {
        if (message != null && signature != null) {
            if (this.maxPlaintextLength() < 32) {
                throw new IllegalArgumentException("modulus is too small for a digital signature");
            } else {
                byte[] hashed = h.eval(message);
                byte[] decryptedSignature = this.decrypt(signature);
                if (decryptedSignature == null) {
                    return false;
                } else if (hashed.length != decryptedSignature.length) {
                    return false;
                } else {
                    for(int i = 0; i < hashed.length; ++i) {
                        if (hashed[i] != decryptedSignature[i]) {
                            return false;
                        }
                    }

                    return true;
                }
            }
        } else {
            throw new NullPointerException();
        }
    }

    public int maxPlaintextLength() {
        int max = (this.modulus.bitLength() - 1) / 8 - 16 - 16 - 2;
        return max < 0 ? 0 : max;
    }

    private byte[] oaep(byte[] plaintext) {
        if (plaintext == null) {
            throw new NullPointerException();
        } else {
            byte[] padded = pad(plaintext, this.maxPlaintextLength() + 1, true);
            byte[] left1 = pad(padded, padded.length + 16, false);
            byte[] rightRandom = new byte[16];
            gen.nextBytes(rightRandom);
            byte[] right1 = pad(rightRandom, 32, false);
            PRGen g = new PRGen(right1);
            byte[] gOutput = new byte[left1.length];
            g.nextBytes(gOutput);
            byte[] left2 = xor(left1, gOutput);
            byte[] hOutput = h.eval(left2);
            byte[] hOutputCropped = new byte[16];
            System.arraycopy(hOutput, 0, hOutputCropped, 0, 16);
            byte[] right2 = xor(hOutputCropped, rightRandom);
            byte[] output = new byte[left2.length + right2.length + 1];
            output[0] = 1;
            System.arraycopy(left2, 0, output, 1, left2.length);
            System.arraycopy(right2, 0, output, left2.length + 1, right2.length);
            return output;
        }
    }

    private byte[] oaepReverse(byte[] input) {
        if (input == null) {
            throw new NullPointerException();
        } else if (input.length != this.maxPlaintextLength() + 16 + 16 + 2) {
            return null;
        } else {
            byte[] left2 = new byte[input.length - 1 - 16];
            System.arraycopy(input, 1, left2, 0, left2.length);
            byte[] right2 = new byte[16];
            System.arraycopy(input, left2.length + 1, right2, 0, right2.length);
            byte[] hOutput = h.eval(left2);
            byte[] hOutputCropped = new byte[16];
            System.arraycopy(hOutput, 0, hOutputCropped, 0, 16);
            byte[] rightRandom = xor(hOutputCropped, right2);
            byte[] right1 = pad(rightRandom, 32, false);
            PRGen g = new PRGen(right1);
            byte[] gOutput = new byte[left2.length];
            g.nextBytes(gOutput);
            byte[] left1 = xor(left2, gOutput);

            for(int i = left1.length - 16; i < left1.length; ++i) {
                if (left1[i] != 0) {
                    return null;
                }
            }

            return unpad(left1);
        }
    }

    private static byte[] xor(byte[] a, byte[] b) {
        if (a != null && b != null) {
            if (a.length != b.length) {
                throw new IllegalArgumentException("a,b must have the same length");
            } else {
                byte[] c = new byte[a.length];

                for(int i = 0; i < c.length; ++i) {
                    c[i] = (byte)(a[i] ^ b[i]);
                }

                return c;
            }
        } else {
            throw new NullPointerException();
        }
    }

    private static byte[] pad(byte[] a, int length, boolean marker) {
        if (a == null) {
            throw new NullPointerException();
        } else if (length < a.length) {
            throw new IllegalArgumentException("length must be >= a.length");
        } else if (marker && length < a.length + 1) {
            throw new IllegalArgumentException("length must be >= a.length+1 to have a marker byte");
        } else {
            byte[] padded = new byte[length];
            System.arraycopy(a, 0, padded, 0, a.length);
            if (marker) {
                padded[a.length] = 1;
            }

            return padded;
        }
    }

    private static byte[] unpad(byte[] a) {
        if (a == null) {
            throw new NullPointerException();
        } else {
            int unpaddedLength;
            for(unpaddedLength = a.length - 1; a[unpaddedLength] == 0; --unpaddedLength) {
            }

            if (unpaddedLength <= 0) {
                throw new IllegalArgumentException("a is nothing but padding");
            } else {
                byte[] unpadded = new byte[unpaddedLength];
                System.arraycopy(a, 0, unpadded, 0, unpaddedLength);
                return unpadded;
            }
        }
    }
}
