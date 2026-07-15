# Scenario Forge

**Scenario Forge** is an AI simulation framework for building intelligent agents using **Goal-Oriented Action Planning (GOAP)** instead of hand-authored behavior trees or finite state machines.

The project is designed for simulations where agents need to make flexible, explainable decisions without requiring developers to manually define thousands of transitions. Scenario Forge allows agents to evaluate goals, select actions, and fall back to alternate objectives when plans fail.

Rather than forcing designers to tune every behavior by hand, Scenario Forge aims to use algorithms to adjust agent priorities and behavioral weights toward a desired objective.

Scenario Forge can be used as a foundation for specialized AI systems in games and simulations.

## Tagline

**Explainable GOAP-based AI for adaptive agents and simulations.**

## Reasoning and Planning

Each agent owns a Reasoner and a Planner. The Reasoner selects the highest-scoring eligible Goal from the agent sheet, and the Planner builds an action sequence that satisfies that Goal's desired world states.

## Style Sheets

![Style Sheets](docs/images/behavior_style_sheet.png)

Style sheets define the set of actions and behaviors available to an AI character. Designers can enable or disable individual behavior flags to control what that AI is permitted to do. Each AI character can be assigned its own style sheet, allowing behavior capabilities to be customized on a per-agent basis.

This lets designers create distinct AI roles without rewriting behavior logic. For example, one agent can be allowed to advance, take cover, and throw grenades, while another can be limited to defending, suppressing, or holding a fixed position. Style sheets keep AI behavior authoring readable by separating what an agent is allowed to do from the lower-level decision logic that chooses when to do it.
