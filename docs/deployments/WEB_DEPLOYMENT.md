# Website Duild and Deployment

This document describes the end-to-end deployment strategy for the fheroes2 site deployment.
We target Github Pages for hosting and leverage GitHub Actions for automation.

## Functionality

* Builds the documentation site for the fheroes2 project
* Deploys the site, along with WebAssembly build, to Github Pages on push.
* Allows end users to view the documentation via the web.
* Allows end users to click a link and launch the game via the web.

## Infrastructure Diagram

```mermaid
C4Context
    title FHeroes2 WebAssembly Infrastructure
    Person(user, "End User", "Plays the game via web browser")
    Person(committer, "Engineer", "Engineer who commits into master")

    System_Boundary(github-pages, "GitHub Pages") {
        System(site, "Website", "Github pages hosted website")
    }
    System_Boundary(github, "GitHub") {
        System(repo, "GitHub Repository", "Source code and infrastructure")
    }
    System_Boundary(github-releases, "GitHub Releases") {
        System(releases, "GitHub Releases", "Stores build artifacts")
    }
    System_Boundary(github_actions, "GitHub Actions") {
        System(push_action, "Push Action ", "Triggers the build of the application")
        System(make_action, "Make Action ", "Builds the releases including emscripten release")
        System(pages_action, "Pages Action", "Builds and deploys the pages site")
    }

    Rel(committer, repo, "Pushes change into master")
    Rel(repo, push_action, "Triggers workflows", "On push")
    Rel(push_action, make_action, "Builds Applicaion", "workflow_call")
    Rel(make_action, releases, "Uploads Releases", "latest commit")
    Rel(push_action, pages_action, "Builds site", "workflow_run")
    Rel(releases, pages_action, "Downloads release")
    Rel(pages_action, site, "Deploys Site")
    Rel(user, site, "Plays game", "HTTPS")
```

## Deployment Pipeline Overview

### Trigger

* Deployment is triggered **when a new Git commit is pushed**.
* A GitHub Action workflow listens via `workflow_run` to wait for the build-and-release process.

### Build Process (Upstream)

* Compiles the FHeroes2 WebAssembly bundle using Emscripten.
* Packages assets and build artifacts into a ZIP file.
* Publishes the ZIP as a GitHub Release asset using `ncipollo/release-action`.

### Deploy Process (Downstream)

* Listens for completion of the upstream `Make` workflow.
* Pulls the latest GitHub Release and extracts the ZIP bundle.
* Builds and Deploys the Github Pages with the bundle.

## GitHub Workflows Summary

### make.yml

* Triggered on `push` to branch
* Builds the application
* Publishes release assets

### pages.yml

* Triggered on `workflow_run` completion of `push.yml`
* Builds Jekyll site
* Downloads release
* Deploys to Github Pages

## Future Considerations

* Hook up a custom domain if needed
* Allow better devex around pages/emscripten process for local builds
