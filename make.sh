echo Building able-live
cd able-live
g++ -o able-live -I . -I headers \
    AbleClient.cpp \
    AbleServer.cpp \
    able_live.cpp \
    ChartCoords.cpp \
    Chart.cpp \
    ChartFile.cpp \
    -lpthread -lcurl -lallegro -lallegro_primitives -lallegro_image -lallegro_font -lallegro_ttf -lallegro_dialog || exit
strip able-live
echo Done
