#include <iostream>
#include <string>
#include "Serial.h"
using namespace std;

// Steuerzeichen
static const unsigned char SOH = 0x01; // Start Of Header // static const unsigned --> unveränderliche 8-Bit-Byte-Konstante (0..255) aber nur "char" geht auch
const char ETX = 0x03; // Padding
const char EOT = 0x04; // End Of Transmission
const char ACK = 0x06; // Acknowledge
const char NAK = 0x15; // No Acknowledge

// Blocklayout (vereinfachte Übung)
// | SOH | n | 255-n | Daten(5) | Checksum |
//    1     1    1        5          1      = 9
static const int DATABYTES = 5;
static const int BLOCKSIZE = 3 + DATABYTES + 1; // 9

unsigned char checksum5(unsigned char* d) {
    int sum = 0;
    for (int i = 0; i < DATABYTES; ++i) {
        sum += d[i];
    }
    return (unsigned char)(sum % 256);
}

int main() {
    string portNr;
    cout << "COM Port Nummer: ";
    //cin >> portNr;
    portNr = "2";
    string port = "COM" + portNr;

    // Reihenfolge: (port, baud, dataBits, stopBits, parity)
    Serial com(port, 9600, 8, ONESTOPBIT, NOPARITY);

    // 0. Schritt
    if (!com.open()) {
        cout << "Konnte " << port << " nicht oeffnen.\n";
        return 1;
    }

    // 1a. Schritt -  Empfaenger signalisiert: bereit (a. ==> es wird gesendet; b. ==> es wird empfangen)
    cout << "Empfaenger gestartet auf " << port << "\n";
    com.write(NAK); 

    string nachricht;

    while (true) {
        int b0 = com.read(); // 2b. Schritt - lese nur >>1<< Byte von den 9 Bytes aus dem Puffer
        if (b0 == EOT) {     // 4b. Schritt - Sender beendet
            com.write(ACK);  // 5a. Schritt - wenn ...
            break;
        }
        if (b0 != SOH) continue;             // warte auf Start eines Blocks

        unsigned char blk[BLOCKSIZE];
        blk[0] = (unsigned char)b0;

        // Restliche 8 Bytes einlesen (n, 255-n, 5 Daten, checksum)
        for (int i = 1; i < BLOCKSIZE; ++i) {
            int bi = com.read();
            blk[i] = (unsigned char)bi;
        }

        // Header prüfen
        unsigned char n = blk[1]; 
        cout << "Block " << (int)n << endl;
        unsigned char inv = blk[2];
        if ((unsigned char)(255 - n) != inv) {
            com.write(NAK);// 3a. Schritt - wenn Header falsch
            continue;
        }

        // Pruefsumme pruefen
        unsigned char calc = checksum5(&blk[3]);
        cout << "Checksumme: " << (int)blk[8] << endl;
        if (calc != blk[8]) {
            com.write(NAK); // 3a. Schritt - wenn Pruefsumme falsch
            continue;
        }

        // Daten übernehmen (ETX nicht übernehmen)
        for (int i = 0; i < DATABYTES; ++i) {
            unsigned char c = blk[3 + i];
            if (c != ETX) nachricht.push_back((unsigned char)c);
        }

        com.write(ACK); // 3a. Schritt - alles okay, naechster Block
    }

    cout << "Empfangene Nachricht: " << nachricht << "\n";
    com.close(); // 6. Schritt
    return 0;
}
