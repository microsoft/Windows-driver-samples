import yaml
import os

# Read the CODEOWNERS file
codeowners_path = os.path.join(os.path.dirname(__file__), "..", "CODEOWNERS")
with open(codeowners_path, "r") as file:
    lines = file.readlines()

# Parse the CODEOWNERS file to extract areas and their paths
areas = []
sample_section_found = False

for line in lines:
    line = line.strip()
    if line.startswith("# Samples"):
        sample_section_found = True
        continue

    if sample_section_found:
        if line.startswith("#"):
            continue
        elif line:
            path, codeowner = line.split()
            if path in areas:
                raise ValueError(f"Path:{path} has been found two times inside CODEOWNERS file")
            areas.append(path)


# Sort the areas in lexicographical order
areas = sorted(areas)

# Generate the YAML structure
yaml_form = {
    "name": "Issue with a sample",
    "description": "Report a problem you have with a specific sample.",
    "title": "[path/to/sample]: ",
    "body": []
}

dropdown = {
    "type": "dropdown",
    "id": "sample_area",
    "attributes": {
        "label": "Which is the area where the sample lives?",
        "description": "Select the area where you're experiencing the problem.",
        "options": areas
    },
    "validations": {
        "required": True
    }
}

# Add a description field
description_field = {
    "type": "textarea",
    "id": "description",
    "attributes": {
        "label": "Describe the issue",
        "description": "Provide a clear and concise description of what the issue is."
    },
    "validations": {
        "required": True
    }
}

yaml_form["body"].append(dropdown)
yaml_form["body"].append(description_field)

# Write the YAML to a file
output_path = os.path.join(os.path.dirname(__file__), "sample_issue.yml")
with open(output_path, "w") as outfile:
    yaml.dump(yaml_form, outfile, sort_keys=False)
