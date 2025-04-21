const loginForm = document.querySelector("#login-form");
if (loginForm) {
  loginForm.addEventListener("submit", (e) => {
    e.preventDefault();
    const errorElement = document.getElementById("error-message");
    errorElement.innerHTML = "";

    const email = loginForm["input-email"].value.trim();
    const password = loginForm["input-password"].value;

    if (!email) {
      errorElement.innerHTML = "Please enter your email address.";
      return;
    }
    if (!email.includes("@") || !email.includes(".")) {
      errorElement.innerHTML = "Please enter a valid email address.";
      return;
    }
    if (!password) {
      errorElement.innerHTML = "Please enter your password.";
      return;
    }

    auth
      .signInWithEmailAndPassword(email, password)
      .then((cred) => {
        loginForm.reset();
      })
      .catch((error) => {
        let friendlyErrorMessage = "Login failed. Please try again.";
        switch (error.code) {
          case "auth/user-not-found":
            friendlyErrorMessage = "No account found with this email.";
            break;
          case "auth/wrong-password":
            friendlyErrorMessage = "Incorrect password.";
            break;
          case "auth/too-many-requests":
            friendlyErrorMessage = "Too many attempts. Try again later.";
            break;
          case "auth/invalid-email":
            friendlyErrorMessage = "Invalid email format.";
            break;
          case "auth/user-disabled":
            friendlyErrorMessage = "This account has been disabled.";
            break;
        }
        errorElement.innerHTML = friendlyErrorMessage;
      });
  });
}
