<?php
include "./db_conn.php";

if ($_SERVER['REQUEST_METHOD'] == "POST") {
    $amount = $_POST['amount']; //get the amount from the form
    $transaction_id = time();
    $reads_card = 0; //card read is false here

    // --- SECURE PREPARED STATEMENT ---
    $stmt = $conn->prepare("INSERT INTO payment (transaction_id, amount, reads_card) VALUES (?, ?, ?)");
    $stmt->bind_param("idi", $transaction_id, $amount, $reads_card);
    
    if ($stmt->execute()) {
        // --- THIS IS THE FIX ---
        // Redirect to the status page, but add the new transaction ID to the URL
        header("Location: ../Frontend/approved_transaction_page.html?tid=" . $transaction_id);
    } else {
        echo "Error: " . $stmt->error;
    }

    $stmt->close();
    $conn->close();
}
?>