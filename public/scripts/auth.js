document.addEventListener("DOMContentLoaded", function () {
  // listen for auth status changes
  auth.onAuthStateChanged((user) => {
    if (user) {
      console.log("user logged in");
      console.log(user);
      setupUI(user);
      var uid = user.uid;
      console.log(uid);
    } else {
      console.log("user logged out");
      setupUI();
    }
  });

  // login
  const loginForm = document.querySelector("#login-form");
  loginForm.addEventListener("submit", (e) => {
    e.preventDefault();
    const errorElement = document.getElementById("error-message");
    errorElement.innerHTML = "";

    const email = loginForm["input-email"].value.trim();
    const password = loginForm["input-password"].value;

    // Basic validation
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
        window.location.href = "dashboard.html";
        const user = cred.user;
        localStorage.setItem("loggedInUserId", user.uid);
      })
      .catch((error) => {
        let friendlyErrorMessage = "Login failed. Please try again.";
        console.log("Auth error:", error);

        switch (error.code) {
          case "auth/user-not-found":
            friendlyErrorMessage = "No account found with this email.";
            break;
          case "auth/wrong-password":
            friendlyErrorMessage = "Incorrect password. Please try again.";
            break;
          case "auth/too-many-requests":
            friendlyErrorMessage =
              "Too many attempts. Account temporarily locked. Try again later.";
            break;
          case "auth/invalid-email":
            friendlyErrorMessage = "Invalid email format.";
            break;
          case "auth/user-disabled":
            friendlyErrorMessage = "This account has been disabled.";
            break;
          case "auth/internal-error":
            if (error.message.includes("INVALID_LOGIN_CREDENTIALS")) {
              friendlyErrorMessage =
                "Invalid email or password. Please check your credentials.";
            }
            break;
        }

        errorElement.innerHTML = friendlyErrorMessage; // <-- put this here
      });
  });
});

/* document.addEventListener("DOMContentLoaded", function () {
  const registerForm = document.getElementById("register-form");

  if (registerForm) {
    registerForm.addEventListener("submit", function (e) {
      console.log("Register form submitted");
      e.preventDefault();

      const firstName = document.getElementById("input-firstname").value;
      const lastName = document.getElementById("input-lastname").value;
      const email = document.getElementById("input-email").value;
      const password = document.getElementById("input-password").value;

      firebase
        .auth()
        .createUserWithEmailAndPassword(email, password)
        .then((userCredential) => {
          const user = userCredential.user;
          return firebase
            .database()
            .ref("UsersData/" + user.uid)
            .set({
              firstName,
              lastName,
              email,
            });
        })
        .then(() => {
          window.location.href = "index.html";
        })
        .catch((error) => {
          console.error("Registration error:", error);
          document.getElementById("error-message-register").textContent =
            error.message;
        });
    });
  }
}); */

// logout
const logout = document.querySelector("#logout-button");

if (logout) {
  logout.addEventListener("click", (e) => {
    e.preventDefault();
    // Sign out the user from Firebase
    firebase
      .auth()
      .signOut()
      .then(() => {
        // Redirect the user to login page after successful logout
        window.location.href = "index.html"; // Redirect to login page after logout
      })
      .catch((error) => {
        console.error("Error logging out: ", error);
      });
  });
}
