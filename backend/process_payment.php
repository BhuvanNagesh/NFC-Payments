<?php
//include the file with the database connection
include "./db_conn.php";

// 1. CHECK FOR CONNECTION ERROR FIRST
if ($conn->connect_error) {
    echo "Error: DB Connection Failed: " . $conn->connect_error;
    exit(); // Stop script
}

//this must be the same api key with the one in the arduino
$api_key = "somade_daniel";

//this function is used to do a little sanitizing of user input
function sanitizeInput($input)
{
    return htmlspecialchars(stripslashes(trim($input)));
}

// Check api key
if ($api_key != strval($_GET["apikey"])) {
    echo "Incorrect Api Key which = " . strval($_GET["apikey"]);
    exit(); // Stop
}

// Check request method
if ($_SERVER["REQUEST_METHOD"] != "GET") {
    echo "Invalid method of sending data";
    exit(); // Stop
}

// --- ALL CHECKS PASSED, PROCEED ---

// 1. Get data from Arduino (THIS IS THE NEW PART)
$card_number = sanitizeInput($_GET["card_number"]);
$payment_type = sanitizeInput($_GET["paymentType"]);

// --- THIS IS THE FIX ---
// We must get the transaction ID from the ESP
if (!isset($_GET['tid'])) {
    echo "Error: Transaction ID (tid) was not sent from ESP.";
    exit();
}
$transaction_id = (int)$_GET['tid'];
// --- END OF FIX ---


// 2. Find the student by card_number
$stmt = $conn->prepare("SELECT * FROM students_data WHERE card_number = ?");
$stmt->bind_param("s", $card_number);
$stmt->execute();
$fetch_res = $stmt->get_result();

if ($fetch_res->num_rows > 0) {
    // Card was found
    $card = $fetch_res->fetch_assoc();
    $card_balance = $card['balance'];
    $card_new_balance = $card_balance;

    // 3. Find the pending transaction amount from the `payment` table
    // --- THIS IS THE FIX ---
    // Instead of "LIMIT 1", we find the *exact* transaction
    $trans_stmt = $conn->prepare("SELECT amount FROM payment WHERE transaction_id = ? AND reads_card = 0");
    $trans_stmt->bind_param("i", $transaction_id);
    $trans_stmt->execute();
    $check_transation_res = $trans_stmt->get_result();
    // --- END OF FIX ---
    
    if ($check_transation_res && $check_transation_res->num_rows > 0) {
        $check_transation = $check_transation_res->fetch_assoc();
        $amount_to_pay = $check_transation['amount'];
        // $transaction_id is already defined above

        // ==========================================================
        // THIS IS THE CRITICAL LOGIC
        // ==========================================================
        if ($payment_type == "debit") {
            // --- DEBIT LOGIC ---
            if ($card_balance < $amount_to_pay) {
                echo "Insufficient fund, balance = #" . $card_new_balance;
            } else {
                $card_new_balance = $card_balance - $amount_to_pay;
                // Pass the $transaction_id to the update function
                updateBalance($conn, $card_number, $amount_to_pay, $card_balance, $card_new_balance, "Payment is successful", "1", $transaction_id);
            }
        } else {
            // --- CREDIT LOGIC (Default) ---
            $card_new_balance = $card_balance + $amount_to_pay;
            // Pass the $transaction_id to the update function
            updateBalance($conn, $card_number, $amount_to_pay, $card_balance, $card_new_balance, "Account credited", "0", $transaction_id);
        }
        // ==========================================================

    } else {
        echo "No amount stated or transaction already processed";
    }
    $trans_stmt->close();
} else {
    echo "Card not registered";
}

$stmt->close();
$conn->close();


// --- MODIFIED updateBalance function ---
// It now takes an INTEGER for transactionId
function updateBalance($dbConn, $cardNumber, $amount, $previousBalance, $currentBalance, $returnMessage, $transactionType, $transactionId)
{
    // Use prepared statements
    $update_sql = $dbConn->prepare("UPDATE students_data SET balance = ? WHERE card_number = ?");
    $update_sql->bind_param("ds", $currentBalance, $cardNumber);

    $log_update = $dbConn->prepare("INSERT INTO logs (card_number, transaction_type, amount, previous_balance, balance) VALUES (?, ?, ?, ?, ?)");
    $log_update->bind_param("ssddd", $cardNumber, $transactionType, $amount, $previousBalance, $currentBalance);

    // --- THIS IS THE FIX ---
    // This query now updates the *exact* transaction, setting reads_card=1
    $transaction_update = $dbConn->prepare("UPDATE payment SET reads_card=1 WHERE transaction_id = ?");
    $transaction_update->bind_param("i", $transactionId); // "i" for integer
    // --- END OF FIX ---

    //check if updating of balance worked
    if ($update_sql->execute() && $log_update->execute() && $transaction_update->execute()) {
        echo $returnMessage . ", new balance= #" . $currentBalance;
    } else {
        echo "Error updating records: " . $dbConn->error;
    }
    
    $update_sql->close();
    $log_update->close();
    $transaction_update->close();
}
?>