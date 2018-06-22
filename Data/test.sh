./build/sinsyNG -o xtest4.wav Test4.xml
./build/sinsyNG -o xtest3.wav Test3.xml
./build/sinsyNG -o xtest2.wav Test2.xml
./build/sinsyNG -o xtest.wav Test.xml
sox xtest*.wav test_musicxml.wav
./build/sinsyNG -u test.sinsy -o test_uscore.wav

