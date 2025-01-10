#!/bin/bash
SCRIPT_DIR=$(dirname "$(realpath "$0")")
cd $SCRIPT_DIR

if [[ ! -f .env ]]; then
  echo ".env doesn't exist"
  exit 1
fi
export $(grep -v '^#' .env | xargs)

if [[ -z "$LAT" || -z "$LON" || -z "$KEY" ]]; then
  echo "Please make sure LAT LON and API KEY are defined in .env file"
  exit 1
fi

# Endpoint API
URL="https://api.openweathermap.org/data/3.0/onecall?lat=$LAT&lon=$LON&units=metric&exclude=minutely,hourly,daily,alerts&appid=$KEY"

RESPONSE=$(curl -s "$URL")

if [[ -z "$RESPONSE" ]]; then
  echo "API Download failure!"
  exit 1
fi

OUTPUT_FILE="output.json"
echo "$RESPONSE" >> "$OUTPUT_FILE"