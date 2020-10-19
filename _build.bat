@cls
@TITLE C Sample Dining Philosopher Problem

docker stop dp-sample-instance
docker rm   dp-sample-instance

docker build -t dp-sample-container:latest .
docker run --rm -ti --privileged -v c:\repos\DiningPhilosopherProblemInC\src\:/src:rw -p 6543:6543/tcp --name dp-sample-instance dp-sample-container