<?php
include "./db_conn.php";

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $matric_number = $_POST['matric_number'];
    $name = $_POST['name'];
    $card_id = $_POST['card_id'];
    $amount = $_POST['amount'];
    $password = $_POST['password']; // Storing plain text password (insecure)
    
    // --- SECURE PREPARED STATEMENT ---
    // 1. Prepare the query
    $stmt = $conn->prepare("INSERT INTO students_data (matric_number, name, card_number, balance, password) VALUES (?, ?, ?, ?, ?)");
    // 2. Bind variables (s = string, d = double/decimal)
    $stmt->bind_param("sssis", $matric_number, $name, $card_id, $amount, $password);
    
    // 3. Execute
    if ($stmt->execute()) {
        header("Location: ../Frontend/users.php");
    } else {
        echo "Error: " . $stmt->error;
    }
    
    $stmt->close();
    $conn->close();
}
?>