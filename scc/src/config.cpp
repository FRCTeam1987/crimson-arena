#include "config.h"

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include "status_led.h"

namespace
{

    constexpr uint8_t kSdMisoPin = 5;
    constexpr uint8_t kSdMosiPin = 6;
    constexpr uint8_t kSdSclkPin = 7;
    constexpr uint8_t kSdCsPin = 4;

    constexpr const char *kConfigPath = "/scc.cfg";
    constexpr const char *kDefaultConfigContents = "# location of SCC. valid values are RED, BLUE, and SCORING\nlocation=RED\n";

    SPIClass sdSpi(HSPI);

    String TrimmedLine(String line)
    {
        line.trim();
        return line;
    }

    bool ParseLocation(String value, SccLocation *location)
    {
        value.toUpperCase();
        if (value == "RED")
        {
            *location = SccLocation::RED;
        }
        else if (value == "BLUE")
        {
            *location = SccLocation::BLUE;
        }
        else if (value == "SCORING")
        {
            *location = SccLocation::SCORING;
        }
        else
        {
            return false;
        }
        return true;
    }

    void CreateDefaultConfigFile()
    {
        File file = SD.open(kConfigPath, FILE_WRITE);
        if (!file)
        {
            HaltWithError("Failed to create config file on SD card.");
        }
        file.print(kDefaultConfigContents);
        file.close();
        Serial.println("Config file not found. Created default config.");
    }

}

const char *GetSCCLocationString(SccLocation location)
{
    switch (location)
    {
    case SccLocation::RED:
        return "red";
    case SccLocation::BLUE:
        return "blue";
    case SccLocation::SCORING:
        return "scoring";
    }
    return "unknown";
}

Config LoadConfig()
{
    pinMode(kSdMisoPin, INPUT_PULLUP);
    sdSpi.begin(kSdSclkPin, kSdMisoPin, kSdMosiPin, kSdCsPin);

    if (!SD.begin(kSdCsPin, sdSpi))
    {
        HaltWithError("SD card not detected. Halting.");
    }

    if (!SD.exists(kConfigPath))
    {
        CreateDefaultConfigFile();
    }

    File file = SD.open(kConfigPath, FILE_READ);
    if (!file)
    {
        HaltWithError("Failed to open config file on SD card.");
    }

    Config config{};
    bool locationSet = false;

    while (file.available())
    {
        String line = TrimmedLine(file.readStringUntil('\n'));
        if (line.length() == 0 || line.startsWith("#"))
        {
            continue;
        }

        int separatorIndex = line.indexOf('=');
        if (separatorIndex == -1)
        {
            continue;
        }

        String key = TrimmedLine(line.substring(0, separatorIndex));
        String value = TrimmedLine(line.substring(separatorIndex + 1));

        if (key.equalsIgnoreCase("location"))
        {
            if (!ParseLocation(value, &config.location))
            {
                file.close();
                HaltWithError("Invalid 'location' in config file. Must be RED, BLUE, or SCORING.");
            }
            locationSet = true;
        }
    }
    file.close();

    if (!locationSet)
    {
        HaltWithError("Config file is missing the 'location' setting.");
    }

    return config;
}
