echo Building able-live
cd able-live
g++ -o able-live -I . -I headers \
    src/able_live.cpp \
    src/AbleClient.cpp \
    src/ChartCoords.cpp \
    src/Chart.cpp \
    src/ChartFile.cpp \
    src/PilotAwareFetch.cpp \
    src/FR24Fetch.cpp \
    src/GniusServer.cpp \
    -lpthread -lcurl -lallegro -lallegro_primitives -lallegro_image -lallegro_font -lallegro_ttf -lallegro_dialog || exit
strip able-live
echo Done
