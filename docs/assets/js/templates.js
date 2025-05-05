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

/**
 * HTML templates for the heading-copy and code-copy functionality
 */

// Create the fheroes2 namespace if it doesn't exist
window.fheroes2 = window.fheroes2 || {};

// Template for the live region
const liveRegionTemplate = `
<div aria-live="assertive" aria-atomic="true" style="position: absolute; width: 1px; height: 1px; padding: 0; margin: -1px; overflow: hidden; clip: rect(0, 0, 0, 0); white-space: nowrap; border: 0;"></div>
`;

// Template for the heading anchor icon
const headingAnchorTemplate = `
<span class="heading-anchor" aria-label="Copy link to heading" role="button" tabindex="0">🔗</span>
`;

// Template for the code copy button
const codeCopyButtonTemplate = `
<button class="btn btn-primary copy-btn" aria-label="Copy code to clipboard" title="Copy code to clipboard" tabindex="0">Copy</button>
`;

// Export the templates to the fheroes2 namespace
window.fheroes2.HeadingCopyTemplates = {
    liveRegionTemplate,
    headingAnchorTemplate
};

window.fheroes2.CodeCopyTemplates = {
    liveRegionTemplate,
    codeCopyButtonTemplate
};
