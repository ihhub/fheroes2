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

// Utility class for handling common accessibility features
class AccessibilityManager {
    constructor() {
        this.liveRegion = this.createLiveRegion();
    }

    // Creates a live region for screen reader announcements
    // Returns: The live region element
    createLiveRegion() {
        const liveRegion = document.createElement('div');
        liveRegion.setAttribute('aria-live', 'assertive');
        liveRegion.setAttribute('aria-atomic', 'true');
        liveRegion.style.cssText = 'position: absolute; width: 1px; height: 1px; padding: 0; margin: -1px; overflow: hidden; clip: rect(0, 0, 0, 0); white-space: nowrap; border: 0;';
        document.body.appendChild(liveRegion);
        return liveRegion;
    }

    // Announces a message to screen readers
    // Parameters:
    //   message - The message to announce
    announce(message) {
        // Clear the live region first
        this.liveRegion.textContent = '';

        // Force a reflow
        void this.liveRegion.offsetHeight;

        // Set the new message
        this.liveRegion.textContent = message;

        // Force another reflow to ensure the announcement is triggered
        void this.liveRegion.offsetHeight;
    }

    // Creates an accessible element (button or span)
    // Parameters:
    //   text - The element text
    //   ariaLabel - The ARIA label
    //   className - Additional CSS classes
    //   elementType - The element type ('button' or 'span')
    // Returns: The accessible element
    createAccessibleElement(text, ariaLabel, className = '', elementType = 'button') {
        const element = document.createElement(elementType);
        element.textContent = text;
        element.setAttribute('aria-label', ariaLabel);
        element.setAttribute('role', 'button');
        element.setAttribute('tabindex', '0');
        if (className) {
            element.className = className;
        }
        return element;
    }

    // Creates an accessible button element
    // Parameters:
    //   text - The button text
    //   ariaLabel - The ARIA label
    //   className - Additional CSS classes
    // Returns: The button element
    createAccessibleButton(text, ariaLabel, className = '') {
        return this.createAccessibleElement(text, ariaLabel, className, 'button');
    }

    // Creates an accessible span element
    // Parameters:
    //   text - The span text
    //   ariaLabel - The ARIA label
    //   className - Additional CSS classes
    // Returns: The span element
    createAccessibleSpan(text, ariaLabel, className = '') {
        return this.createAccessibleElement(text, ariaLabel, className, 'span');
    }

    // Updates element state with visual and screen reader feedback
    // Parameters:
    //   element - The element to update
    //   success - Whether the action was successful
    //   successMessage - Message for screen reader on success
    //   errorMessage - Message for screen reader on error
    //   successText - Text to show on element for success
    //   errorText - Text to show on element for error
    //   defaultText - Default element text
    //   resetDelay - Delay before resetting element (ms)
    //   isHtml - Whether the text contains HTML
    updateElementState(element, success, successMessage, errorMessage, successText, errorText, defaultText, resetDelay = 2000, isHtml = false) {
        // Update visual state first
        if (success) {
            if (isHtml) {
                element.innerHTML = successText;
            } else {
                element.textContent = successText;
            }
            element.classList.add('success');
        } else {
            if (isHtml) {
                element.innerHTML = errorText;
            } else {
                element.textContent = errorText;
            }
            element.classList.add('error');
        }

        // Announce to screen reader after a short delay to ensure visual update is complete
        setTimeout(() => {
            this.announce(success ? successMessage : errorMessage);
        }, 50);

        // Reset element after delay
        setTimeout(() => {
            if (isHtml) {
                element.innerHTML = defaultText;
            } else {
                element.textContent = defaultText;
            }
            element.classList.remove('success', 'error');
        }, resetDelay);
    }

    // Sets up keyboard event handling for an element
    // Parameters:
    //   element - The element to add keyboard events to
    //   handler - The click handler function
    setupKeyboardEvents(element, handler) {
        element.addEventListener('keydown', (event) => {
            if (event.key === 'Enter' || event.key === ' ') {
                event.preventDefault();
                handler(event);
            }
        });
    }
}

// Export the accessibility manager
window.AccessibilityManager = AccessibilityManager;
