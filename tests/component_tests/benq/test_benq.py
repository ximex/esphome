"""Tests for the benq component."""

from collections.abc import Callable

from tests.component_tests.helpers import extract_packed_value


def test_benq_lamp_time_sensor_metadata(generate_main: Callable[[str], str]) -> None:
    """LAMP_TIME must carry the metadata Home Assistant needs for statistics.

    Without a unit and a state class, Home Assistant raises repair issues for the
    entity once it has recorded long term statistics for it.
    """
    main_cpp = generate_main("tests/component_tests/benq/test_benq.yaml")

    # A raw string here would not compile: set_state_class takes an enum.
    assert "set_state_class(sensor::STATE_CLASS_TOTAL_INCREASING)" in main_cpp
    assert "set_accuracy_decimals(0)" in main_cpp

    # Unit, device class, category and icon are packed into the register call.
    # Codegen spells them out in the trailing comment.
    assert extract_packed_value(main_cpp, "lamp_hours") != 0
    assert "uom:h" in main_cpp
    assert "dc:duration" in main_cpp
    assert "category:diagnostic" in main_cpp


def test_benq_defaults_do_not_override_user_values(
    generate_main: Callable[[str], str],
) -> None:
    """Values given in the yaml win over the per-command defaults."""
    main_cpp = generate_main("tests/component_tests/benq/test_benq_override.yaml")

    assert "set_state_class(sensor::STATE_CLASS_MEASUREMENT)" in main_cpp
    assert "set_accuracy_decimals(2)" in main_cpp
    assert "uom:min" in main_cpp


def test_benq_number_command_defaults(generate_main: Callable[[str], str]) -> None:
    """A number gets the range and the slider mode of its command.

    The defaults have to be applied before the schema runs: ``mode`` carries a
    default of its own there, so a later pass would never see it missing, and
    the raw string would not compile.
    """
    main_cpp = generate_main("tests/component_tests/benq/test_benq_entities.yaml")

    assert "set_mode(number::NUMBER_MODE_SLIDER)" in main_cpp
    assert "set_min_value(0)" in main_cpp
    assert "set_max_value(10)" in main_cpp


def test_benq_select_gets_options_of_its_command(
    generate_main: Callable[[str], str],
) -> None:
    """A select without options would be an entity nobody can use."""
    main_cpp = generate_main("tests/component_tests/benq/test_benq_entities.yaml")

    assert 'set_options({"4:3", "16:9", "16:10", "AUTO", "REAL"})' in main_cpp


def test_benq_entities_know_hub_and_command(
    generate_main: Callable[[str], str],
) -> None:
    """Hub and command are required and never change, so they are constructor
    arguments. Entities add themselves to the hub from there."""
    main_cpp = generate_main("tests/component_tests/benq/test_benq_entities.yaml")

    assert "benq::BenqSwitch(benq_hub, benq::BenqCommand::BLANK)" in main_cpp
    assert "benq::BenqNumber(benq_hub, benq::BenqCommand::VOLUME)" in main_cpp
    assert "benq::BenqSelect(benq_hub, benq::BenqCommand::ASPECT_RATIO)" in main_cpp
