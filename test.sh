# Test script for plagiarism detection system

echo "=== Test 1: Basic cheat transformation ==="
cat > /tmp/test1.p << 'EOF'
(function (fibonacci n)
  (if (<= n 1)
    n
    (+ (fibonacci (- n 1)) (fibonacci (- n 2)))))

(function (main)
  (print (fibonacci 10)))
endprogram
EOF

echo "Original program:"
cat /tmp/test1.p
echo ""
echo "Transformed program:"
./code < /tmp/test1.p
echo ""

echo "=== Test 2: Anticheat - identical structure ==="
cat > /tmp/test2.txt << 'EOF'
(function (add x y)
  (+ x y))

(function (main)
  (print (add 5 10)))
endprogram
(function (sum a b)
  (+ a b))

(function (main)
  (print (sum 5 10)))
endprogram
EOF

echo "Similarity (should be high ~1.0):"
./code < /tmp/test2.txt
echo ""

echo "=== Test 3: Anticheat - different programs ==="
cat > /tmp/test3.txt << 'EOF'
(function (add x y)
  (+ x y))

(function (main)
  (print (add 5 10)))
endprogram
(function (factorial n)
  (if (<= n 1)
    1
    (* n (factorial (- n 1)))))

(function (main)
  (print (factorial 5)))
endprogram
EOF

echo "Similarity (should be lower):"
./code < /tmp/test3.txt
echo ""

echo "=== Test 4: Cheat + Anticheat round-trip ==="
./code < /tmp/test1.p > /tmp/transformed.p
echo "endprogram" >> /tmp/transformed.p
cat /tmp/test1.p /tmp/transformed.p > /tmp/roundtrip.txt
echo "Similarity between original and transformed:"
./code < /tmp/roundtrip.txt
echo ""

echo "=== All tests completed ==="
