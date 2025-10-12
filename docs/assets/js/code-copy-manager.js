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

// Class to handle code copy functionality
class CodeCopyManager
{
    // Static instance of AccessibilityManager
    static accessibility = null;

    // Static initialization method
    static initialize()
    {
        // Initialize accessibility manager if not already initialized
        if ( !this.accessibility ) {
            this.accessibility = new window.fheroes2.AccessibilityManager();
        }

        // Apply to all code blocks
        document.querySelectorAll( "div.highlight" ).forEach( block => this.initializeCodeBlock( block ) );
    }

    // Creates a copy button using the template
    // Returns: The button element
    static createCopyButton()
    {
        return this.accessibility.createAccessibleButton( { text : 'Copy', ariaLabel : 'Copy code to clipboard', className : 'btn btn-primary copy-btn' } );
    }

    // Updates the button appearance and announces success to screen readers
    // Parameters:
    //   button - The button element
    //   success - Whether the copy was successful
    //   code - The code content that was copied
    static updateButtonState( button, success, code )
    {
        // Truncate the code if it's too long
        const truncatedCode = code.length > 50 ? `${code.substring( 0, 50 )}...` : code;

        this.accessibility.updateElementState( {
            element : button,
            success : success,
            successMessage : `Code copied to clipboard: ${truncatedCode}`,
            errorMessage : 'Failed to copy code to clipboard',
            successText : 'Copied!',
            errorText : 'Failed to copy',
            defaultText : 'Copy',
            resetDelay : 2000
        } );
    }

    // Handles the copy functionality for a code block
    // Parameters:
    //   block - The code block element
    //   button - The copy button
    static handleCopyCode( block, button )
    {
        try {
            const code = block.querySelector( "pre" ).innerText;
            navigator.clipboard.writeText( code ).then( () => { this.updateButtonState( button, true, code ); } ).catch( err => {
                console.error( "Failed to copy code:", err );
                this.updateButtonState( button, false, code );
            } );
        }
        catch ( err ) {
            console.error( "Failed to copy code:", err );
            this.updateButtonState( button, false, '' );
        }
    }

    // Sets up event listeners for a copy button
    // Parameters:
    //   button - The copy button
    //   block - The code block element
    static setupEventListeners( button, block )
    {
        // Click event handler
        button.addEventListener( "click", () => this.handleCopyCode( block, button ) );

        // Keyboard event handler
        this.accessibility.setupKeyboardEvents( button, () => this.handleCopyCode( block, button ) );
    }

    // Initializes a code block with a copy button
    // Parameters:
    //   block - The code block element
    static initializeCodeBlock( block )
    {
        const button = this.createCopyButton();
        block.appendChild( button );
        this.setupEventListeners( button, block );
    }
}

// Initialize when the DOM is fully loaded
document.addEventListener( "DOMContentLoaded", () => { CodeCopyManager.initialize(); } );
