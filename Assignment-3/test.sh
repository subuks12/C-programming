binary=build/image_calc

make clean; make

if [[ ! -a $binary ]]; then
    echo "FAIL: $binary not made"
    echo "SCORE: 0/2"
    exit 0
fi
score=0
# Function to run the image operation and compare the output with the reference
check_image_operation() {
    local input_image=$1
    local width=$2
    local height=$3
    local output_image=$4
    local reference_image=$5

    # Run the operation
    echo "./build/image_calc $input_image $width $height $output_image"
    ./build/image_calc "$input_image" "$width" "$height" "$output_image" > /dev/null
    


    if [[ ! -a $output_image ]]; then
        echo "FAIL: $output_image not made"
        return
    fi
    # Compare the output with the reference
    local diff_result=$(diff "$output_image" "$reference_image" | wc -l)

    # Check if the diff result is non-zero (meaning there's a mismatch)
    if [ "$diff_result" -ne 0 ]; then
        echo "$operation Image Operation FAIL: Produced image does not match reference."
    else
        echo "$operation Image Operation PASS"
        score=$((score+1))
    fi
}

# Check greenscreen operation
check_image_operation "071537020427" 113 42 "barcode-1.bmp" "reference/barcode-1.bmp"
check_image_operation "071641818033" 113 42 "barcode-2.bmp" "reference/barcode-2.bmp"


echo "TEST: valgrind ./build/image_calc \"071537020427\" 113 42 \"barcode-1.bmp\" "
valgrind --leak-check=full --error-exitcode=1   ./build/image_calc "071537020427" 113 42 "barcode-1.bmp" > /dev/null

if [ "$?" -eq 0 ]; then
    ((score+=1))
else
    echo "  --FAIL!"
    echo "  Valgrind found memory errors:"
    cat valgrind.log
fi

echo "TEST: plotted images are present"
if [ -f "plot.png" ]; then
    ((score+=1))
else
    echo "  --FAIL!"
fi

echo "TEST: questions.txt is non-empty and contains questions (1) through (N)"
N=3
if [ -f questions.txt ] && [ "$(wc -c < questions.txt)" -gt 200 ]; then
    passed=true
    for ((i=1; i<=N; i++)); do
        if ! grep -q "^($i)" questions.txt; then
            echo "  --FAIL: questions.txt missing ($i)"
            passed=false
        fi
    done
    if $passed; then
        ((score+=1))
    fi
else
    echo "  --FAIL!"
fi

echo "SCORE: $score/5"
