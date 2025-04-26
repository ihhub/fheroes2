/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// Theme toggle functionality
class ThemeManager
{
    // Static properties
    static themeToggle = null;
    static themeKey = "theme";
    static darkTheme = "dark";
    static lightTheme = "light";
    static accessibility = null;

    // Static initialization method
    static initialize()
    {
        // Initialize accessibility manager if not already initialized
        if ( !this.accessibility ) {
            this.accessibility = window.fheroes2.AccessibilityManager ? new window.fheroes2.AccessibilityManager() : null;
        }

        this.themeToggle = document.getElementById( "theme-toggle" );
        this.applySavedTheme();
        this.setupEventListeners();
        this.setupAccessibility();
    }

    static setTheme( theme, saveToStorage = true )
    {
        document.documentElement.setAttribute( "data-theme", theme );
        document.body.setAttribute( "data-theme", theme );

        // Only save to localStorage if explicitly requested
        if ( saveToStorage ) {
            localStorage.setItem( this.themeKey, theme );
        }

        // Update button text and ARIA attributes
        if ( this.themeToggle ) {
            this.themeToggle.textContent = theme === this.darkTheme ? "☀️ Light Mode" : "🌙 Dark Mode";

            // Update ARIA attributes
            this.themeToggle.setAttribute( "aria-label", `Switch to ${theme === this.darkTheme ? this.lightTheme : this.darkTheme} mode` );
            this.themeToggle.setAttribute( "aria-pressed", theme === this.darkTheme ? "true" : "false" );

            // Announce theme change if accessibility manager is available
            if ( this.accessibility ) {
                this.accessibility.announce( `${theme === this.darkTheme ? 'Dark' : 'Light'} mode activated` );
            }
        }
    }

    static applySavedTheme()
    {
        const savedTheme = localStorage.getItem( this.themeKey );

        if ( savedTheme ) {
            this.setTheme( savedTheme );
        }
        else {
            // Check system preference but don't save it to localStorage
            const prefersDark = window.matchMedia( "(prefers-color-scheme: dark)" ).matches;
            this.setTheme( prefersDark ? this.darkTheme : this.lightTheme, false );
        }
    }

    static setupEventListeners()
    {
        // Add event listener to the toggle button
        if ( this.themeToggle ) {
            this.themeToggle.addEventListener( "click", () => {
                const currentTheme = document.documentElement.getAttribute( "data-theme" );
                this.setTheme( currentTheme === this.darkTheme ? this.lightTheme : this.darkTheme );
            } );
        }

        // Listen for system theme changes
        window.matchMedia( "(prefers-color-scheme: dark)" ).addEventListener( "change", ( e ) => {
            // Only apply system preference if user hasn't manually set a preference
            if ( !localStorage.getItem( this.themeKey ) ) {
                this.setTheme( e.matches ? this.darkTheme : this.lightTheme, false );
            }
        } );
    }

    static setupAccessibility()
    {
        if ( this.themeToggle ) {
            // Set initial ARIA attributes
            const currentTheme = document.documentElement.getAttribute( "data-theme" );
            this.themeToggle.setAttribute( "aria-label", `Switch to ${currentTheme === this.darkTheme ? this.lightTheme : this.darkTheme} mode` );
            this.themeToggle.setAttribute( "aria-pressed", currentTheme === this.darkTheme ? "true" : "false" );

            // Add keyboard event handling
            this.themeToggle.addEventListener( "keydown", ( event ) => {
                if ( event.key === "Enter" || event.key === " " ) {
                    event.preventDefault();
                    const currentTheme = document.documentElement.getAttribute( "data-theme" );
                    this.setTheme( currentTheme === this.darkTheme ? this.lightTheme : this.darkTheme );
                }
            } );
        }
    }
}

// Initialize the theme manager when the DOM is loaded
document.addEventListener( "DOMContentLoaded", () => { ThemeManager.initialize(); } );
