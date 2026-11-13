# Contributing to DJV

Thank you for helping. Bug reports, sample files that show a problem, and
pull requests are all welcome.

## Before a pull request

Open an issue first for anything beyond a small fix: a new feature, a change
to how something behaves, or a change to the build. Agreeing on what to do
before the code is written saves both of us a rewrite, and some requests are
already planned or done differently than they look from outside.

## Pull requests

- **One change per pull request.** A fix and an unrelated clean up are two
  pull requests.
- **Tests with every change**, and the existing tests passing. New behavior
  goes in the models (`lib/djv/Models`) with a test in `lib/djv/ModelsTest`;
  see [AGENTS.md](AGENTS.md) for where things live.
- **Run it, not only build it.** Say what you ran it on (platform, graphics,
  media) and what you checked. A change to a platform's behavior needs to
  have been run on that platform.
- **Keep it small.** A pull request that can be read in one sitting is one
  that gets merged. Large features are better split, or discussed first.
- **No new dependencies** without discussing them in an issue first.
- **Against `main`**, kept up to date with it. Draft pull requests with no
  activity for three months may be closed; reopening is always fine.

## However it was written

Code written with an AI assistant is welcome on the same terms as any other:
the person who sends a change stands behind it. That means you have read it,
you understand it, you can answer questions about it, and you have run it.
[AGENTS.md](AGENTS.md) describes the project for assistants and people
alike.

## License

DJV is BSD-3-Clause licensed. By sending a pull request you agree that your
change is licensed the same way.
