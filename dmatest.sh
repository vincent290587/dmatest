#!/bin/bash

# Configuration - Match these to your Vivado Address Editor
DMA_REG_BASE=0x40400000
SRC_PHYS=0x30000000
DST_PHYS=0x34000000
LEN=128  # Start small (128 bytes) to avoid "Width of Buffer Length" errors

# Register Offsets
MM2S_DMACR=0x00
MM2S_DMASR=0x04
MM2S_SA=0x18
MM2S_LENGTH=0x28

S2MM_DMACR=0x30
S2MM_DMASR=0x34
S2MM_DA=0x48
S2MM_LENGTH=0x58

echo "--- AXI DMA Loopback Test ---"

# 1. Reset the DMA
echo "Resetting DMA..."
devmem $((DMA_REG_BASE + MM2S_DMACR)) 32 0x4
devmem $((DMA_REG_BASE + S2MM_DMACR)) 32 0x4
sleep 1 # Give it a moment to clear

# 2. Clear Destination Memory (for verification)
echo "Clearing destination memory..."
for i in {0..3}; do
    devmem $((DST_PHYS + i*4)) 32 0x0
done

# 3. Set up Source Data
echo "Writing test pattern to source..."
devmem $((SRC_PHYS)) 32 0xDEADBEEF
devmem $((SRC_PHYS + 4)) 32 0xCAFEBABE

# 4. Start the Channels (Set the Run/Stop bit to 1)
echo "Starting Channels..."
devmem $((DMA_REG_BASE + MM2S_DMACR)) 32 0x1
devmem $((DMA_REG_BASE + S2MM_DMACR)) 32 0x1

# 5. Provide Addresses
devmem $((DMA_REG_BASE + MM2S_SA)) 32 $SRC_PHYS
devmem $((DMA_REG_BASE + S2MM_DA)) 32 $DST_PHYS

# 6. TRIGGER THE TRANSFER
# IMPORTANT: Always set S2MM (Receive) length BEFORE MM2S (Transmit)
echo "Triggering Transfer..."
devmem $((DMA_REG_BASE + S2MM_LENGTH)) 32 $LEN
devmem $((DMA_REG_BASE + MM2S_LENGTH)) 32 $LEN

# 7. Wait and Check Status
sleep 1
MM2S_STATUS=$(devmem $((DMA_REG_BASE + MM2S_DMASR)))
S2MM_STATUS=$(devmem $((DMA_REG_BASE + S2MM_DMASR)))

echo "MM2S Status: $MM2S_STATUS"
echo "S2MM Status: $S2MM_STATUS"

# 8. Verify Data
RESULT=$(devmem $DST_PHYS)
echo "Data at Destination: $RESULT"

if [ "$RESULT" == "0xDEADBEEF" ]; then
    echo "SUCCESS: Loopback confirmed!"
else
    echo "FAILURE: Check Address Editor or DMA IRQs."
fi
