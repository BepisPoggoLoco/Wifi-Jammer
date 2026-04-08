#include <SPI.h>
#include <RF24.h>

// Pin config (CE, CSN)
RF24 radio(9, 10); 

// Max payload size for nRF24L01 is exactly 32 bytes
uint8_t payload[32];
uint32_t packetCount = 0;

bool writing;

void setup() {
  Serial.begin(115200);
  Serial.println(F("RF24 Max-Throughput Transmitter"));

  radio.begin();

  radio.setDataRate(RF24_2MBPS);        // Fastest air rate
  radio.setPALevel(RF24_PA_MAX);        // Max TX power for range
  radio.setPayloadSize(32);            // Full 32-byte payload every packet
  radio.setCRCLength(RF24_CRC_8);      // 8-bit CRC is faster than 16-bit (still safe for short-range)
  radio.setRetries(0, 0);             // NO retries — broadcast doesn't need ACKs
  radio.setAutoAck(false);            // Disable ACK entirely — fire and forget

  // Dynamic payloads add overhead — use fixed size for max throughput
  radio.disableDynamicPayloads();

  // Print config to serial for debugging
  radio.printDetails();
}

void loop() {
}

void JAM(){
  while (writing){
    // Fill payload with useful data (replace with your own data struct)
    
    packetCount++;
    memcpy(payload, &packetCount, 4);           // First 4 bytes = packet number
    memset(payload + 4, 0xAB, 28);              // Remaining 28 bytes = data
  
    // writeFast() queues into the TX FIFO without waiting for ACK,
    radio.writeFast(payload, sizeof(payload));
  
    // Flush the FIFO every ~3 packets to avoid stalls
    if (packetCount % 3 == 0) {
      radio.txStandBy();
    }
  
    if (packetCount % 1000 == 0) {
      Serial.print(F("Packets sent: "));
      Serial.println(packetCount);
    }
  }
}

void STOP_JAMMING(){
  writing = false;
}

void START_JAMMING(){
  writing = true;
}

bool RSSI_ANALYSIS(int channel){
  radio.setChannel(channel);
  radio.startListening();
  
  delayMicroseconds(150);

  bool channel_RSSI = radio.testRPD();
  radio.stopListening();

  return channel_RSSI; 
}
