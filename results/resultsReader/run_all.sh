java -jar target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar --input ../Sesimbra_07_Jan_2012/theirs/v2/20130207_1100e.msd --outputWithTimeSinceEpoch --data-output results/e1
java -jar target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar --input ../Sesimbra_07_Jan_2012/theirs/v2/20130207_1200e.msd --outputWithTimeSinceEpoch --data-output results/e2
java -jar target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar --input ../Sesimbra_07_Jan_2012/theirs/v2/20130207_1300e.msd --outputWithTimeSinceEpoch --data-output results/e3

java -jar target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar --input ../Sesimbra_07_Jan_2012/theirs/v2/20130207_1100n.msd --outputWithTimeSinceEpoch --data-output results/n1
java -jar target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar --input ../Sesimbra_07_Jan_2012/theirs/v2/20130207_1200n.msd --outputWithTimeSinceEpoch --data-output results/n2
java -jar target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar --input ../Sesimbra_07_Jan_2012/theirs/v2/20130207_1300n.msd --outputWithTimeSinceEpoch --data-output results/n3

java -jar target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar --input ../Sesimbra_07_Jan_2012/theirs/v2/20130207_1100z.msd --outputWithTimeSinceEpoch --data-output results/z1
java -jar target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar --input ../Sesimbra_07_Jan_2012/theirs/v2/20130207_1200z.msd --outputWithTimeSinceEpoch --data-output results/z2
java -jar target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar --input ../Sesimbra_07_Jan_2012/theirs/v2/20130207_1300z.msd --outputWithTimeSinceEpoch --data-output results/z3

cd results
cat e1 e2 e3 > e
cat n1 n2 n3 > n
cat z1 z2 z3 > z
cd ..

# java -jar target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar --input ../Sesimbra_07_Jan_2012/ours/miavita.json.1 --isInputJson --outputWithTimeSinceEpoch --channel 1 --data-output results/oursN
# java -jar target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar --input ../Sesimbra_07_Jan_2012/ours/miavita.json.1 --isInputJson --outputWithTimeSinceEpoch --channel 2 --data-output results/oursE
# java -jar target/myReader-1.0-SNAPSHOT-jar-with-dependencies.jar --input ../Sesimbra_07_Jan_2012/ours/miavita.json.1 --isInputJson --outputWithTimeSinceEpoch --channel 3 --data-output results/oursZ
