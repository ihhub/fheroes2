// Theme toggle functionality
class ThemeManager {
    constructor() {
        this.themeToggle = document.getElementById("theme-toggle");
        this.themeKey = "theme";
        this.darkTheme = "dark";
        this.lightTheme = "light";
        this.accessibility = window.AccessibilityManager ? new window.AccessibilityManager() : null;

        this.init();
    }

    init() {
        this.applySavedTheme();
        this.setupEventListeners();
        this.setupAccessibility();
    }

    setTheme(theme, saveToStorage = true) {
        document.documentElement.setAttribute("data-theme", theme);
        document.body.setAttribute("data-theme", theme);

        // Only save to localStorage if explicitly requested
        if (saveToStorage) {
            localStorage.setItem(this.themeKey, theme);
        }

        // Update button text and ARIA attributes
        if (this.themeToggle) {
            this.themeToggle.textContent = theme === this.darkTheme
                ? "☀️ Light Mode"
                : "🌙 Dark Mode";

            // Update ARIA attributes
            this.themeToggle.setAttribute("aria-label", `Switch to ${theme === this.darkTheme ? this.lightTheme : this.darkTheme} mode`);
            this.themeToggle.setAttribute("aria-pressed", theme === this.darkTheme ? "true" : "false");

            // Announce theme change if accessibility manager is available
            if (this.accessibility) {
                this.accessibility.announce(`${theme === this.darkTheme ? 'Dark' : 'Light'} mode activated`);
            }
        }
    }

    applySavedTheme() {
        const savedTheme = localStorage.getItem(this.themeKey);

        if (savedTheme) {
            this.setTheme(savedTheme);
        } else {
            // Check system preference but don't save it to localStorage
            const prefersDark = window.matchMedia("(prefers-color-scheme: dark)").matches;
            this.setTheme(prefersDark ? this.darkTheme : this.lightTheme, false);
        }
    }

    setupEventListeners() {
        // Add event listener to the toggle button
        if (this.themeToggle) {
            this.themeToggle.addEventListener("click", () => {
                const currentTheme = document.documentElement.getAttribute("data-theme");
                this.setTheme(currentTheme === this.darkTheme ? this.lightTheme : this.darkTheme);
            });
        }

        // Listen for system theme changes
        window.matchMedia("(prefers-color-scheme: dark)").addEventListener("change", (e) => {
            // Only apply system preference if user hasn't manually set a preference
            if (!localStorage.getItem(this.themeKey)) {
                this.setTheme(e.matches ? this.darkTheme : this.lightTheme, false);
            }
        });
    }

    setupAccessibility() {
        if (this.themeToggle) {
            // Set initial ARIA attributes
            const currentTheme = document.documentElement.getAttribute("data-theme");
            this.themeToggle.setAttribute("aria-label", `Switch to ${currentTheme === this.darkTheme ? this.lightTheme : this.darkTheme} mode`);
            this.themeToggle.setAttribute("aria-pressed", currentTheme === this.darkTheme ? "true" : "false");

            // Add keyboard event handling
            this.themeToggle.addEventListener("keydown", (event) => {
                if (event.key === "Enter" || event.key === " ") {
                    event.preventDefault();
                    const currentTheme = document.documentElement.getAttribute("data-theme");
                    this.setTheme(currentTheme === this.darkTheme ? this.lightTheme : this.darkTheme);
                }
            });
        }
    }
}

// Initialize the theme manager when the DOM is loaded
document.addEventListener("DOMContentLoaded", () => {
    new ThemeManager();
});
