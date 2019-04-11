set -e

mkdir -p results

while read nodes ; do
    while read seed ; do
        cmd="./centre klondike \
            --settings settings.json \
            --dealseed $seed \
            --limitnodes $nodes"
        echo "$cmd"
        /bin/time -pvo results/$seed-$nodes.time -- $cmd \
            2>results/$seed-$nodes.err | tee results/$seed-$nodes.out
    done < seeds.txt
done < limits.txt
