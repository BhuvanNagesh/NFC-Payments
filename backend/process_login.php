<?php
include "./db_conn.php";
session_start();

// Check if the form has been submitted
if ($_SERVER["REQUEST_METHOD"] == "POST" && isset($_POST['password'])) {
    // Get the form data
    $username = $_POST['username'];
    $password = $_POST['password'];

    // --- SECURE PREPARED STATEMENT ---
    // 1. Prepare the query
    $stmt = $conn->prepare("SELECT * FROM students_data WHERE matric_number = ?");
    // 2. Bind the variable (s = string)
    $stmt->bind_param("s", $username);
    // 3. Execute the query
    $stmt->execute();
    // 4. Get the result
    $result = $stmt->get_result();

    if ($result->num_rows == 1) {
        $rec = $result->fetch_assoc();
        $dbpword = $rec['password'];

        // --- PASSWORD VERIFICATION ---
        // Your current code compares plain text. This is insecure.
        // For a real-world project, you should store passwords with password_hash()
        // and check them with password_verify($password, $dbpword).
        // For now, your original logic is kept:
        if ($password === $dbpword) {
            $_SESSION['username'] = $rec['matric_number'];
            $_SESSION['logged_in'] = true;
            if ($rec['name'] == "admin") {
                header("Location: ../Frontend/dashboard.php");
            } else {
                header("Location: ../Frontend/students"); // Note: This "students" page was not in your file list
            }
            exit(); // Always exit after a header redirect
        } else {
            header("location: ../Frontend/index.php?err=Invalid Login Details!!");
            exit();
        }
    } else {
        header("location: ../Frontend/index.php?err=User does not exist!!");
        exit();
    }
    
    $stmt->close();
    $conn->close();
}
?>