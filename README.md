# ULSOpenLib
UAVLAS Open Library

## Working on the library from several projects

ULSQX, ULSTools and ULSToolsX each embed this repository as a submodule, so by
default each carries its own clone and a change has to travel commit → push →
fetch → checkout → gitlink bump before the other projects see it. Two things
remove that round trip on a development machine.

**A shared working tree.** Every build resolves the library through one
variable, so all of them can be pointed at a single checkout and see an edit
immediately, with no commit at all:

```sh
export ULSOPENLIB_ROOT=$HOME/Projects/UAVLAS/ULSOpenLib-dev
```

ULSQX and ULSTools also accept it as `cmake -DULSOPENLIB_ROOT=...`; ULSToolsX's
Flutter hook, `build_wasm.sh` and `make_sim_book.py` read the environment
variable, and `devs-nrf/build.sh` forwards it into the Zephyr build. Unset, each
project builds its own submodule — which is what a clean clone and CI do, so
nothing about the released artefacts changes.

The trade-off is that what you build is then no longer what the submodule
records. Bumping the gitlink becomes a deliberate step rather than something
every edit demands.

**A local exchange point.** `.libhub/ULSOpenLib.git` is a bare repository beside
the projects, registered in each submodule as the remote `hub`. Commits move
between the projects through it at disk speed and offline; GitHub is reached
only when publishing.

`Scripts/ulslib-sync` drives both:

```
ulslib-sync            # where every copy stands
ulslib-sync share      # shared tree -> hub -> every submodule (the everyday one)
ulslib-sync publish    # the above, then push dev to GitHub
ulslib-sync fetch      # bring GitHub's dev down and spread it
```

`share` fast-forwards each project's submodule and stages the new gitlink; it
never merges and never resolves a divergence on its own. Commit the staged
pointer in each project when the change is ready to be recorded there.
